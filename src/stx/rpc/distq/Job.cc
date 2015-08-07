/*
 * This file is part of the "libstx" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/stdtypes.h"
#include "stx/rpc/distq/Job.h"

using namespace stx;

namespace stx {
namespace distq {

Job::Job(
    const String& job_name) :
    name_(job_name),
    ready_(false),
    error_(false) {}

void Job::cancel() {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_) {
    return;
  }

  error_ = true;
  ready_ = true;
  lk.unlock();

  cv_.notify_all();

  if (on_cancel_) {
    on_cancel_();
  }

  if (on_error_) {
    on_error_("Job cancelled");
  }
}

bool Job::isCancelled() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return error_;
}

void Job::onCancel(Function<void ()> fn) {
  std::unique_lock<std::mutex> lk(mutex_);

  if (error_) {
    lk.unlock();
    fn();
  } else {
    on_cancel_ = fn;
  }
}

void Job::wait() const {
  std::unique_lock<std::mutex> lk(mutex_);

  while (!ready_) {
    cv_.wait(lk);
  }
}

bool Job::waitFor(const Duration& timeout) const {
  std::unique_lock<std::mutex> lk(mutex_);

  if (ready_) {
    return true;
  }

  cv_.wait_for(lk, std::chrono::microseconds(timeout.microseconds()));
  return ready_;
}

HashMap<String, double> Job::getCounters() const {
  std::unique_lock<std::mutex> lk(mutex_);
  return counters_;
}

double Job::getCounter(const String& counter) const {
  std::unique_lock<std::mutex> lk(mutex_);

  auto iter = counters_.find(counter);
  if (iter == counters_.end()) {
    return 0;
  } else {
    return iter->second;
  }
}

//double getProgress() const;
//void onProgress(Function<void (double progress)> fn);

void Job::sendError(const String& error) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (ready_) {
    RAISE(kRuntimeError, "refusing to send an error to a finished job");
  }

  ready_ = true;
  error_ = true;
  lk.unlock();

  cv_.notify_all();

  if (on_error_) {
    on_error_(error);
  }
}

} // namespace distq
} // namespace stx

