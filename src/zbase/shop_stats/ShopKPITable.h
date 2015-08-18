/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "zbase/core/TSDBTableScan.h"
#include "zbase/Report.h"
#include "zbase/VTable.h"
#include "zbase/ProtoSSTableSource.h"
#include "zbase/ShopStats.pb.h"
#include "zbase/shop_stats/ShopCTRStatsScan.h"
#include "zbase/JoinedSession.pb.h"

using namespace stx;

namespace cm {

class ShopKPITable :
    public VTableSource,
    public AbstractProtoSSTableSource<ShopKPIs> {
public:

  ShopKPITable(
      const ReportParams& params,
      tsdb::TSDBService* tsdb);

  Vector<String> columns() const override;
  void forEach(
      Function<bool (const Vector<csql::SValue>&)> fn) override;

  List<dproc::TaskDependency> dependencies() const override;
  void read(dproc::TaskContext* ctx) override;

  const ShopKPIs& aggregates() const;

protected:
  ReportParams params_;
  tsdb::TSDBService* tsdb_;
  Option<Duration> time_window_;
  Function<bool (const Vector<csql::SValue>&)> foreach_;
  ShopKPIs aggr_;
};

} // namespace cm