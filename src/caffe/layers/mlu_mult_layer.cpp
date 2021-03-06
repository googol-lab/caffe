/*
All modification made by Cambricon Corporation: © 2018--2019 Cambricon Corporation
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

#ifdef USE_MLU
#include <vector>
#include "caffe/layers/mlu_mult_layer.hpp"

namespace caffe {

template <typename Dtype>
void MLUMultLayer<Dtype>::LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  CHECK_EQ(bottom[0]->num_axes(), bottom[1]->num_axes())
     << "bottom[0] axes must equal bottom[1] ,"
     << bottom[0]->num_axes() << " vs " << bottom[1]->num_axes();
  CHECK(bottom[0]->shape() == bottom[1]->shape())
    << "bottom[0] and bottom[1] must have the same shape!"
    << bottom[0]->shape_string() << " vs " << bottom[1]->shape_string();
  MultLayer<Dtype>::LayerSetUp(bottom, top);
}

template <typename Dtype>
void MLUMultLayer<Dtype>::Reshape_tensor(const vector<Blob<Dtype>*>& bottom,
                                         const vector<Blob<Dtype>*>& top) {
  MultLayer<Dtype>::Reshape(bottom, top);
}

template <typename Dtype>
void MLUMultLayer<Dtype>::MLUDestroyOp() {
  if (mlu_op_ptr_ != nullptr) {
    MLU_CHECK(cnmlDestroyBaseOp(&mlu_op_ptr_));
    mlu_op_ptr_ = nullptr;
  }
}

template <typename Dtype>
MLUMultLayer<Dtype>::~MLUMultLayer() {
  MLUDestroyOp();
}

template <typename Dtype>
void MLUMultLayer<Dtype>::Forward_mlu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {
  MLU_CHECK(cnmlComputeMultOpForward_V3(mlu_op_ptr_,
                                bottom[0]->mutable_mlu_data(),
                                bottom[1]->mutable_mlu_data(),
                                top[0]->mutable_mlu_data(),
                                Caffe::forward_param(), Caffe::queue()));
}

INSTANTIATE_CLASS(MLUMultLayer);
}  // namespace caffe
#endif
