/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/api/MapReduceService.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/JSONEncoder.h"
#include "sstable/SSTableWriter.h"

using namespace stx;

namespace zbase {

MapReduceService::MapReduceService(
    ConfigDirectory* cdir,
    AnalyticsAuth* auth,
    zbase::TSDBService* tsdb,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl,
    JSRuntime* js_runtime,
    const String& cachedir) :
    cdir_(cdir),
    auth_(auth),
    tsdb_(tsdb),
    pmap_(pmap),
    repl_(repl),
    js_runtime_(js_runtime),
    cachedir_(cachedir) {}

SHA1Hash MapReduceService::mapPartition(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    const String& program_source,
    const String& method_name) {
  logDebug(
      "z1.mapreduce",
      "Executing map shard; partition=$0/$1/$2 output=$3",
      session.customer(),
      table_name,
      partition_key.toString());

  auto table = pmap_->findTable(
      session.customer(),
      table_name);

  if (table.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "table not found: $0/$1/$2",
        session.customer(),
        table_name);
  }

  auto partition = pmap_->findPartition(
      session.customer(),
      table_name,
      partition_key);

  if (partition.isEmpty()) {
    RAISEF(
        kNotFoundError,
        "partition not found: $0/$1/$2",
        session.customer(),
        table_name,
        partition_key.toString());
  }

  auto schema = table.get()->schema();
  auto reader = partition.get()->getReader();

  auto js_ctx = mkRef(new JavaScriptContext());
  js_ctx->loadProgram(program_source);

  auto output_id = Random::singleton()->sha1(); // FIXME
  auto output_path = FileUtil::joinPaths(
      cachedir_,
      StringUtil::format("mr-shard-$0.sst", output_id.toString()));

  auto writer = sstable::SSTableWriter::create(output_path, nullptr, 0);

  reader->fetchRecords([
      &schema,
      &js_ctx,
      &method_name,
      &writer] (
      const Buffer& record) {
    msg::MessageObject msgobj;
    msg::MessageDecoder::decode(record, *schema, &msgobj);
    Buffer msgjson;
    json::JSONOutputStream msgjsons(BufferOutputStream::fromBuffer(&msgjson));
    msg::JSONEncoder::encode(msgobj, *schema, &msgjsons);

    Vector<Pair<String, String>> tuples;
    js_ctx->callMapFunction(method_name, msgjson.toString(), &tuples);

    for (const auto& t : tuples) {
      writer->appendRow(t.first, t.second);
    }
  });

  return output_id;
}

} // namespace zbase