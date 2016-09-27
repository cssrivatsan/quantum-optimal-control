

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include <vector>
#include "dev_array.h"

using namespace tensorflow;
using namespace std;

REGISTER_OP("MatrixExpVecsGrads")
    .Attr("size: int")
    .Attr("input_num: int")
    .Attr("exp_num: int")
    .Attr("vecs_num: int")
    .Input("coeff: float")
    .Input("vecs: float")
    .Input("matrix: float")
    .Output("output: float")
    .Doc(R"doc(
Identitcal as cuda_matexp_vecs_v2.so, but with different name such that graident python file wont be confused, as this operation is required in gradient calculation.
)doc");

void matrixMultiplication(const float* A, const float* B, float* C, const int N, const int M);
void matrixAddV2(const float* A, const float* B, const float* coeff, float* C, const int N);
void matrixAddV3(const float* A, const float* B, const float coeff, float* C, const int N, const int M);

class MatrixExpVecsGradsOp : public OpKernel {
 public:
  explicit MatrixExpVecsGradsOp(OpKernelConstruction* context) : OpKernel(context) {
    OP_REQUIRES_OK(context,
                   context->GetAttr("size", &size_));
  OP_REQUIRES_OK(context,
                   context->GetAttr("input_num", &input_num_));
  OP_REQUIRES_OK(context,
                   context->GetAttr("exp_num", &exp_num_));
  OP_REQUIRES_OK(context,
                   context->GetAttr("vecs_num", &vecs_num_));
}

  void Compute(OpKernelContext* context) override {
    // Grab the input tensor
    const Tensor& input_0_tensor = context->input(0);
    auto input_0 = input_0_tensor.flat<float>();

    const Tensor& input_1_tensor = context->input(1);
    auto vecs = input_1_tensor.flat<float>();

    const Tensor& input_2_tensor = context->input(2);
    auto matrix_ = input_2_tensor.flat<float>();

    const int N = size_*size_;

    // Create an output tensor
    Tensor* output_tensor = NULL;
    OP_REQUIRES_OK(context, context->allocate_output(0, TensorShape({static_cast<int>(sqrt(N)),vecs_num_}),
                                                     &output_tensor));
    auto output = output_tensor->template flat<float>();
 
    int dim = static_cast<int>(sqrt(N));
    const int dim_m = vecs_num_;
    
    const int vecs_size = dim*dim_m;

    dev_array<float> d_m_ii(N);

   
    dev_array<float> d_mat(N);
    dev_array<float> d_mat_temp(N);

    dev_array<float> d_mat_exp(vecs_size);
    dev_array<float> d_mat_exp_temp(vecs_size);
    dev_array<float> d_mat_n(vecs_size);
    dev_array<float> d_mat_n_temp(vecs_size);


    // Call the cuda kernel launcher

    d_mat.set(&matrix_.data()[0], N);

    for (int ii = 1; ii < input_num_; ii++) {
      d_m_ii.set(&matrix_.data()[ii*N], N);
      matrixAddV2(d_mat.getData(), d_m_ii.getData(), &input_0.data()[ii], d_mat_temp.getData(), dim);
      d_mat.set(d_mat_temp.getData(),N);
    }

    d_m_ii.free();
    d_mat_temp.free();

    d_mat_n.set(vecs.data(), vecs_size);
    d_mat_exp.set(vecs.data(), vecs_size);

    float inv_factorial = 1.0;

    for (int num  = 1; num < exp_num_; num++) {
      inv_factorial = inv_factorial/ num;
      matrixMultiplication(d_mat.getData(), d_mat_n.getData(), d_mat_n_temp.getData(), dim, dim_m);
      matrixAddV3(d_mat_exp.getData(), d_mat_n_temp.getData(), inv_factorial, d_mat_exp_temp.getData(), dim, dim_m);
    
      d_mat_n.set(d_mat_n_temp.getData(), vecs_size);
      d_mat_exp.set(d_mat_exp_temp.getData(), vecs_size);
    }
    
    inv_factorial = inv_factorial/ exp_num_;
    matrixMultiplication(d_mat.getData(), d_mat_n.getData(), d_mat_n_temp.getData(), dim, dim_m);
    matrixAddV3(d_mat_exp.getData(), d_mat_n_temp.getData(), inv_factorial, output.data(), dim, dim_m);
    cudaDeviceSynchronize();     

    d_mat.free();

  }

    private:
   int size_;
   int input_num_;
   int exp_num_;
   int vecs_num_;
};

REGISTER_KERNEL_BUILDER(Name("MatrixExpVecsGrads").Device(DEVICE_GPU), MatrixExpVecsGradsOp);

