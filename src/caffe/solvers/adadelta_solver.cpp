/*
All modification made by Cambricon Corporation: © 2018 Cambricon Corporation
All rights reserved.
All other contributions:
Copyright (c) 2014--2018, the respective contributors
All rights reserved.
For the list of contributors go to https://github.com/BVLC/caffe/blob/master/CONTRIBUTORS.md
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <vector>

#include "caffe/sgd_solvers.hpp"

namespace caffe {

template <typename Dtype>
void AdaDeltaSolver<Dtype>::AdaDeltaPreSolve() {
  // Add the extra history entries for AdaDelta after those from
  // SGDSolver::PreSolve
  const vector<Blob<Dtype>*>& net_params = this->net_->learnable_params();
  for (int i = 0; i < net_params.size(); ++i) {
        const vector<int>& shape = net_params[i]->shape();
        this->history_.push_back(
                shared_ptr<Blob<Dtype> >(new Blob<Dtype>(shape)));
  }
}

#ifdef USE_CUDA
template <typename Dtype>
void adadelta_update_gpu(int N, Dtype* g, Dtype* h, Dtype* h2, Dtype momentum,
    Dtype delta, Dtype local_rate);
#endif

template <typename Dtype>
void AdaDeltaSolver<Dtype>::ComputeUpdateValue(int param_id, Dtype rate) {
  const vector<Blob<Dtype>*>& net_params = this->net_->learnable_params();
  const vector<float>& net_params_lr = this->net_->params_lr();
  Dtype delta = this->param_.delta();
  Dtype momentum = this->param_.momentum();
  Dtype local_rate = rate * net_params_lr[param_id];
  size_t update_history_offset = net_params.size();
  switch (Caffe::mode()) {
  case Caffe::CPU: {
    // compute square of gradient in update
    caffe_powx(net_params[param_id]->count(),
        net_params[param_id]->cpu_diff(), Dtype(2),
        this->update_[param_id]->mutable_cpu_data());

    // update history of gradients
    caffe_cpu_axpby(net_params[param_id]->count(), Dtype(1) - momentum,
        this->update_[param_id]->cpu_data(), momentum,
        this->history_[param_id]->mutable_cpu_data());

    // add delta to history to guard against dividing by zero later
    caffe_set(net_params[param_id]->count(), delta,
        this->temp_[param_id]->mutable_cpu_data());

    caffe_add(net_params[param_id]->count(),
        this->temp_[param_id]->cpu_data(),
        this->history_[update_history_offset + param_id]->cpu_data(),
        this->update_[param_id]->mutable_cpu_data());

    caffe_add(net_params[param_id]->count(),
        this->temp_[param_id]->cpu_data(),
        this->history_[param_id]->cpu_data(),
        this->temp_[param_id]->mutable_cpu_data());

    // divide history of updates by history of gradients
    caffe_div(net_params[param_id]->count(),
        this->update_[param_id]->cpu_data(),
        this->temp_[param_id]->cpu_data(),
        this->update_[param_id]->mutable_cpu_data());

    // jointly compute the RMS of both for update and gradient history
    caffe_powx(net_params[param_id]->count(),
        this->update_[param_id]->cpu_data(), Dtype(0.5),
        this->update_[param_id]->mutable_cpu_data());

    // compute the update
    caffe_mul(net_params[param_id]->count(),
        net_params[param_id]->cpu_diff(),
        this->update_[param_id]->cpu_data(),
        net_params[param_id]->mutable_cpu_diff());

    // compute square of update
    caffe_powx(net_params[param_id]->count(),
        net_params[param_id]->cpu_diff(), Dtype(2),
        this->update_[param_id]->mutable_cpu_data());

    // update history of updates
    caffe_cpu_axpby(net_params[param_id]->count(), Dtype(1) - momentum,
        this->update_[param_id]->cpu_data(), momentum,
        this->history_[update_history_offset + param_id]->mutable_cpu_data());

    // apply learning rate
    caffe_cpu_scale(net_params[param_id]->count(), local_rate,
        net_params[param_id]->cpu_diff(),
        net_params[param_id]->mutable_cpu_diff());
    break;
  }
  case Caffe::GPU: {
#ifdef USE_CUDA
    adadelta_update_gpu(net_params[param_id]->count(),
        net_params[param_id]->mutable_gpu_diff(),
        this->history_[param_id]->mutable_gpu_data(),
        this->history_[update_history_offset + param_id]->mutable_gpu_data(),
        momentum, delta, local_rate);
#else
    NO_GPU;
#endif
    break;
  }
  default:
    LOG(FATAL) << "Unknown caffe mode: " << Caffe::mode();
  }
}

INSTANTIATE_CLASS(AdaDeltaSolver);
REGISTER_SOLVER_CLASS(AdaDelta);

}  // namespace caffe
