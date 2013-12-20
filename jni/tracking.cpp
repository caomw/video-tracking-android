/* DO NOT EDIT THIS FILE - it is machine generated */

#define END_PROF(X)
#define START_PROF(X)
#define SCOPE_PROF(X)

#include <ctime>

#include <jni.h>
#include <ctime>
#include <vector>
#include <cstring>
#include <android/log.h>
#include <cuimg/gl.h>
#include <cuimg/draw.h>
#include <cuimg/neighb2d_data.h>
#include <cuimg/cpu/host_image2d.h>
#include <cuimg/cpu/convolve.h>
#include <cuimg/tracking2/tracker.h>

#include "spy_demo_CameraStream.h"
#include "common.hh"

using namespace cuimg;

template <typename I>
void draw_trajectories(std::vector<trajectory>& trajectories, I& out, int traj_len, i_uchar4 color, unsigned factor)
{
  const auto& trs = trajectories;
  for(unsigned i = 0; i < trs.size(); i++)
  {
    const trajectory& traj = trs[i];

    if (traj.history.size() < 1 || !out.has(traj.history.back())) continue;

    for (unsigned k = 1; k < traj.history.size() && k < traj_len; k++)
    {
      unsigned j = traj.history.size() - k - 1;
      draw_line2d(out, traj.history[j] * factor, traj.history[j + 1] * factor, i_uchar4(0,255,0,255));
    }
    draw_c8(out, traj.history.back() * factor, color);
    out(traj.history.back() * factor) = i_uchar4(0,0,0,255);
  }
}

struct environment
{
public:
  typedef tracker<tracking_strategies::bc2s_miel3_gradient_cpu> T;
  environment() { time = get_time(); fps = 0; tr = 0; tracking_time = 0.f; }

  void update(obox2d domain, const unsigned char* yuv, unsigned char* out_rgba)
  {

    if (not (gl_frame.domain() == domain))
    {
      if (tr) delete tr;
      tr = new T(domain, 3);
      tr->scale(0).strategy().detector().set_contrast_threshold(10);
      tr->scale(1).strategy().detector().set_contrast_threshold(10);
      tr->scale(2).strategy().detector().set_contrast_threshold(10);
      tr->strategy().set_detector_frequency(5).set_filtering_frequency(1);

      gl_frame = host_image2d<gl8u>(domain);
      __android_log_print(ANDROID_LOG_ERROR, "tag", "Input size is %i x %i", gl_frame.ncols(), gl_frame.nrows());
    }

    nv21_to_gl(domain.ncols(), domain.nrows(), yuv, (unsigned char*) gl_frame.data());

    double t = get_time();
    tr->run(gl_frame);

    tr->scale(0).pset().sync_attributes(trajectories, trajectory());
    update_trajectories(trajectories, tr->scale(0).pset());

    tracking_time += get_time() - t;
    gl_to_rgba(domain.ncols(), domain.nrows(), (unsigned char*) gl_frame.data(), out_rgba);

    host_image2d<i_uchar4> out((i_uchar4*) out_rgba, domain.nrows(), domain.ncols(), 4 * domain.ncols() * sizeof(unsigned char));
    draw_trajectories(trajectories, out,  3, i_uchar4(0, 0, 255, 255), 1);

    if ((get_time() - time) > 1. && fps)
    {
      time = get_time();
      __android_log_print(ANDROID_LOG_ERROR, "tag", "Tracking xxx time %f ms/frame", tracking_time*1000.f/fps);
      __android_log_print(ANDROID_LOG_ERROR, "tag", "Fps %i, %i points @ %ix%i", fps, tr->pset().size(), domain.ncols(), domain.nrows());
      fps = 0;
      tracking_time = 0.;
    }

    fps++;
  }
private:

  T* tr;
  host_image2d<gl8u> gl_frame;
  double time;
  unsigned fps;
  double tracking_time;

  std::vector<trajectory> trajectories;

};

environment cppenv;

void Java_spy_demo_CameraStream_tracking(JNIEnv* env, jobject thiz, jint width,
					 jint height, jbyteArray yuv, jintArray bgra)
{
  jbyte* _yuv  = env->GetByteArrayElements(yuv, 0);
  jint*  _bgra = env->GetIntArrayElements(bgra, 0);

  unsigned char* in = (unsigned char *)_yuv;
  unsigned char* buf = (unsigned char *)_bgra;

  cppenv.update(obox2d(height, width), in, buf);

  env->ReleaseIntArrayElements(bgra, _bgra, 0);
  env->ReleaseByteArrayElements(yuv, _yuv, 0);
}