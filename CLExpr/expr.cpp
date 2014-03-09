#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <string>
#include <stdint.h>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <Windows.h>
#include <CL/cl.h>
#include <avisynth.h>
#include "parser\parser.h"
#include "clcode.h"


//http://www.khronos.org/message_boards/showthread.php/5912-error-to-string
static const char* get_cl_error_string(cl_int error)
{
    switch (error)
    {
    case CL_SUCCESS:                            return "Success!";
    case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
    case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
    case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
    case CL_OUT_OF_RESOURCES:                   return "Out of resources";
    case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
    case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
    case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
    case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
    case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
    case CL_MAP_FAILURE:                        return "Map failure";
    case CL_INVALID_VALUE:                      return "Invalid value";
    case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
    case CL_INVALID_PLATFORM:                   return "Invalid platform";
    case CL_INVALID_DEVICE:                     return "Invalid device";
    case CL_INVALID_CONTEXT:                    return "Invalid context";
    case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
    case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
    case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
    case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
    case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
    case CL_INVALID_SAMPLER:                    return "Invalid sampler";
    case CL_INVALID_BINARY:                     return "Invalid binary";
    case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
    case CL_INVALID_PROGRAM:                    return "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
    case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
    case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
    case CL_INVALID_KERNEL:                     return "Invalid kernel";
    case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
    case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
    case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
    case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
    case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
    case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
    case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
    case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
    case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
    case CL_INVALID_EVENT:                      return "Invalid event";
    case CL_INVALID_OPERATION:                  return "Invalid operation";
    case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
    case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
    case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
    default: return "Unknown";
    }
}

#define CHECK_CL_ERROR(error, env) \
    if (error != CL_SUCCESS) \
    { \
        env->ThrowError("cl_expr: OpenCL error \"%s\" [line %i]", get_cl_error_string(error), __LINE__); \
    } \

static void replace_first(std::string &source, const std::string &what, const std::string &new_value)
{
    size_t f = source.find(what);
    source.replace(f, what.length(), new_value);
}

enum ExprType
{
    EXPR_X = 1,
    EXPR_XY = 2,
    EXPR_XYZ = 3
};

enum PlaneProcessMode
{
    DO_NOTHING = 1,
    COPY_FIRST = 2,
    PROCESS = 3,
    COPY_SECOND = 4,
    COPY_THIRD = 5
};


static std::string prepare_program(const std::string &expression, ExprType type, bool lsb)
{
    Parser parser = getDefaultParser();
    std::string expr;
    switch (type)
    {
    case EXPR_X:
        parser.addSymbol(Symbol::X);
        expr = lsb ? expr_source_lsb : expr_source;
        break;
    case EXPR_XY:
        parser.addSymbol(Symbol::X).addSymbol(Symbol::Y);
        expr = lsb ? exprxy_source_lsb : exprxy_source;
        break;
    case EXPR_XYZ:
        parser.addSymbol(Symbol::X).addSymbol(Symbol::Y).addSymbol(Symbol::Z);
        expr = lsb ? exprxyz_source_lsb : exprxyz_source;
        break;
    default:
        assert(0);
        break;
    }

    parser.parse(expression, " ");
    Context context(parser.getExpression());

    replace_first(expr, "{{expression}}", context.infix());

    OutputDebugString(expr.c_str());

    std::string program(common_ocl_functions);
    return program + expr;
}

struct PlaneData
{
    int mode;
    std::string expr;
    cl_kernel kernel;
    cl_program program;
    cl_mem dst_buffer;
    cl_mem src_buffers[3];
    cl_command_queue command_queue;
    bool own_program;
    cl_event src_copy_events[3];
    cl_event kernel_run_event;

    PlaneData() : kernel(nullptr), program(nullptr), own_program(false), 
        dst_buffer(nullptr), kernel_run_event(nullptr), command_queue(nullptr)
    {
        for (int i = 0; i < 3; ++i)
        {
            src_buffers[i] = nullptr;
            src_copy_events[i] = nullptr;
        }
    }

    ~PlaneData()
    {
        if (own_program)
        {
            clReleaseProgram(program);
        }
        for (int i = 0; i < 3; i++)
        {
            clReleaseMemObject(src_buffers[i]);
            clReleaseEvent(src_copy_events[i]);
        }
        clReleaseEvent(kernel_run_event);
        clReleaseKernel(kernel);
        clReleaseMemObject(dst_buffer);
        clReleaseCommandQueue(command_queue);
    }

private:
    PlaneData(const PlaneData &other);
    PlaneData operator=(const PlaneData &other);
};

class ClExpr : public GenericVideoFilter
{
public:
    ClExpr(PClip clip1, PClip clip2, PClip clip3,
        std::string expr, std::string yexpr, std::string uexpr, std::string vexpr,
        int y, int u, int v, std::string chroma, bool lsb, ExprType mode, IScriptEnvironment* env);

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;


    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_MULTI_INSTANCE : 0;
    }

    ~ClExpr()
    {
        clReleaseContext(context_);
    }

private:
    cl_context context_;
    PlaneData plane_params_[3];
    PClip clip1_, clip2_, clip3_;
    ExprType filter_type_;
    bool lsb_;
};

static int chroma_to_int(const std::string &chroma)
{
    if (chroma == "process")
        return PROCESS;
    else if (chroma == "copy" || chroma == "copy first")
        return COPY_FIRST;
    else if (chroma == "copy second")
        return COPY_SECOND;
    else if (chroma == "copy third")
        return COPY_THIRD;
    else 
        return -atoi(chroma.c_str());
}

static void to_lower(std::string &data)
{
    std::transform(data.begin(), data.end(), data.begin(), tolower);
}

ClExpr::ClExpr(PClip clip1, PClip clip2, PClip clip3,
    std::string expr, std::string yexpr, std::string uexpr, std::string vexpr,
    int y, int u, int v, std::string chroma, bool lsb, ExprType filter_type, IScriptEnvironment* env)
    : GenericVideoFilter(clip1), context_(nullptr),
    clip1_(clip1), clip2_(clip2), clip3_(clip3), filter_type_(filter_type), lsb_(lsb)
{
    if (!vi.IsPlanar())
    {
        env->ThrowError("cl_expr: only planar color formats supported");
    }
    if (!chroma.empty())
    {
        to_lower(chroma);
        u = v = chroma_to_int(chroma);
    }
    plane_params_[0].mode = y;
    plane_params_[1].mode = u;
    plane_params_[2].mode = v;

    plane_params_[0].expr = yexpr.empty() ? expr : yexpr;
    plane_params_[1].expr = uexpr.empty() ? expr : uexpr;
    plane_params_[2].expr = vexpr.empty() ? expr : vexpr;

    for (int i = 0; i < 3; ++i)
    {
        to_lower(plane_params_[i].expr);
    }

    //init ocl
    cl_platform_id platform;
    cl_int error = clGetPlatformIDs(1, &platform, NULL);
    CHECK_CL_ERROR(error, env);

    cl_device_id device;
    CHECK_CL_ERROR(clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL), env);

    cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
    context_ = clCreateContext(cps, 1, &device, NULL, NULL, &error);
    CHECK_CL_ERROR(error, env);

    const static int planes[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
    int planes_count = (vi.IsPlanar() && !vi.IsY8()) ? 3 : 1;

    for (int i = 0; i < planes_count; i++)
    {
        auto &current = plane_params_[i];
        if (current.mode != PROCESS)
        {
            continue;
        }
        if (current.expr.empty() || current.expr == "x")
        {
            current.mode = COPY_FIRST;
            continue;
        }

        int width = vi.width >> vi.GetPlaneWidthSubsampling(planes[i]);
        int height = vi.height >> vi.GetPlaneHeightSubsampling(planes[i]);
        if (lsb && ((height % 2) != 0))
        {
            env->ThrowError("cl_expr: height of all processed planes with lsb=true must be mod2");
        }
        current.command_queue = clCreateCommandQueue(context_, device, NULL, &error);
        CHECK_CL_ERROR(error, env);

        current.dst_buffer = clCreateBuffer(context_, CL_MEM_WRITE_ONLY, width * height, NULL, &error);
        CHECK_CL_ERROR(error, env);

        for (int j = 0; j < filter_type; j++)
        {
            current.src_buffers[j] = clCreateBuffer(context_, CL_MEM_READ_ONLY, width * height, NULL, &error);
            CHECK_CL_ERROR(error, env);
        }

        for (int prev = 0; prev < i; prev++)
        {
            if (current.expr == plane_params_[prev].expr)
            {
                //some other plane uses the same expression, reuse the program
                current.program = plane_params_[prev].program;
                break;
            }
        }

        if (current.program == nullptr)
        {
            //build new program for this plane to own
            auto source = prepare_program(current.expr, filter_type, lsb_);
            auto cstr = source.c_str();
            
            current.own_program = true;
            current.program = clCreateProgramWithSource(context_, 1, (const char**)&cstr, NULL, &error);
            CHECK_CL_ERROR(error, env);

            error = clBuildProgram(current.program, 0, NULL, NULL, NULL, NULL);
            if (error != CL_SUCCESS)
            {
                size_t len;
                const int buffer_size = 1024*100;
                char buffer[buffer_size];
                memset(buffer, 0, buffer_size);
                clGetProgramBuildInfo(current.program, device, CL_PROGRAM_BUILD_LOG, buffer_size*sizeof(char), buffer, &len);
                OutputDebugString(buffer);

                env->ThrowError(get_cl_error_string(error));
            }
        }

        current.kernel = clCreateKernel(current.program, "expr", &error);
        CHECK_CL_ERROR(error, env);
    }
}

static void copy_source_buffers(PlaneData &pd, const std::vector<std::pair<const uint8_t*, int>> planes,
    int width, int height, IScriptEnvironment* env)
{
    size_t offsets[] = { 0, 0, 0 };
    size_t dimensions[] = { width, height, 1 };
    for (size_t i = 0; i < planes.size(); ++i)
    {
        CHECK_CL_ERROR(clEnqueueWriteBufferRect(pd.command_queue,
            pd.src_buffers[i], CL_FALSE, offsets, offsets, dimensions,
            0, 0, planes[i].second, 0, planes[i].first,
            0, NULL, &pd.src_copy_events[i]), env);
    }
}

static void run_kernel(PlaneData &pd, size_t src_planes_count,
    int width, int height, bool lsb, IScriptEnvironment* env)
{
    int real_height = lsb ? height / 2 : height;
    size_t global_work_size[] = { width * real_height };
    int arg_idx = 0;

    CHECK_CL_ERROR(clSetKernelArg(pd.kernel, arg_idx++, sizeof(cl_mem), &pd.dst_buffer), env);
    for (size_t i = 0; i < src_planes_count; ++i)
    {
        CHECK_CL_ERROR(clSetKernelArg(pd.kernel, arg_idx++, sizeof(cl_mem), &pd.src_buffers[i]), env);
    }
    CHECK_CL_ERROR(clSetKernelArg(pd.kernel, arg_idx++, sizeof(int), &width), env);
    if (lsb)
    {
        CHECK_CL_ERROR(clSetKernelArg(pd.kernel, arg_idx++, sizeof(int), &real_height), env);
    }

    CHECK_CL_ERROR(
        clEnqueueNDRangeKernel(pd.command_queue, pd.kernel, 1, NULL, global_work_size, NULL, 
        src_planes_count, pd.src_copy_events, &pd.kernel_run_event)
        , env);
}

static void copy_dst_buffer(const PlaneData &pd,
    uint8_t* dstp, int dst_pitch,
    int width, int height, IScriptEnvironment* env)
{
    size_t offsets[] = { 0, 0, 0 };
    size_t dimensions[] = { width, height, 1 };

    cl_int error = clEnqueueReadBufferRect(pd.command_queue, pd.dst_buffer, CL_FALSE, 
        offsets, offsets, dimensions, 0, 0, dst_pitch, 0, dstp, 1, &pd.kernel_run_event, NULL);
    CHECK_CL_ERROR(error, env);
}

PVideoFrame ClExpr::GetFrame(int n, IScriptEnvironment* env)
{
    auto dst = env->NewVideoFrame(vi);
    auto src1 = clip1_->GetFrame(n, env);
    auto src2 = clip2_ == nullptr ? nullptr : clip2_->GetFrame(n, env);
    auto src3 = clip3_ == nullptr ? nullptr : clip3_->GetFrame(n, env);

    const static int planes[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
    size_t planes_count = (vi.IsPlanar() && !vi.IsY8()) ? 3 : 1;
    
    for (size_t i = 0; i < planes_count; i++)
    {
        auto& params = plane_params_[i];
        int plane = planes[i];
        if (params.mode == DO_NOTHING)
        {
        }
        else if (params.mode == COPY_FIRST || 
            (params.mode == COPY_SECOND && clip2_ == nullptr) || 
            (params.mode == COPY_THIRD && clip3_ == nullptr))
        {
            env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane),
                src1->GetReadPtr(plane), src1->GetPitch(plane),
                src1->GetRowSize(plane), src1->GetHeight(plane));
        }
        else if (params.mode == COPY_SECOND)
        {
            env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane),
                src2->GetReadPtr(plane), src2->GetPitch(plane),
                src2->GetRowSize(plane), src2->GetHeight(plane));
        }
        else if (params.mode == COPY_THIRD)
        {
            env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane),
                src3->GetReadPtr(plane), src3->GetPitch(plane),
                src3->GetRowSize(plane), src3->GetHeight(plane));
        }
        else if (params.mode <= 0)
        {
            memset(dst->GetWritePtr(plane), -params.mode, dst->GetPitch(plane) * dst->GetHeight(plane));
        }
        else if (params.mode == PROCESS)
        {
            //copy planes to device
            std::vector<std::pair<const uint8_t*, int>> src_planes;

            src_planes.emplace_back(src1->GetReadPtr(plane), src1->GetPitch(plane));
            if (src2 != nullptr)
            {
                src_planes.emplace_back(src2->GetReadPtr(plane), src2->GetPitch(plane));
            }
            if (src3 != nullptr)
            {
                src_planes.emplace_back(src3->GetReadPtr(plane), src3->GetPitch(plane));
            }
            copy_source_buffers(params, src_planes, dst->GetRowSize(plane), dst->GetHeight(plane), env);
        } 
        else
        {
            env->ThrowError("cl_expr: invalid mode. This is a bug");
        }
    }

    //process kernels
    for (size_t i = 0; i < planes_count; i++)
    {
        if (plane_params_[i].mode == PROCESS)
        {
            run_kernel(plane_params_[i], filter_type_, dst->GetRowSize(planes[i]), dst->GetHeight(planes[i]), lsb_, env);
        }
    }

    //copy results back
    for (size_t i = 0; i < planes_count; i++)
    {
        if (plane_params_[i].mode == PROCESS)
        {
            copy_dst_buffer(plane_params_[i], dst->GetWritePtr(planes[i]), dst->GetPitch(planes[i]), dst->GetRowSize(planes[i]), dst->GetHeight(planes[i]), env);
        }
    }

    //wair for everything to complete
    for (size_t i = 0; i < planes_count; i++)
    {
        if (plane_params_[i].mode == PROCESS)
        {
            clFinish(plane_params_[i].command_queue);
        }
    }

    return dst;
}


AVSValue __cdecl create_expr(AVSValue args, void*, IScriptEnvironment* env)
{
    enum { CLIP, EXPR, YEXPR, UEXPR, VEXPR, Y, U, V, CHROMA, LSB };
    return new ClExpr(
        args[CLIP].AsClip(), 
        nullptr, 
        nullptr,
        args[EXPR].AsString(""),
        args[YEXPR].AsString(""),
        args[UEXPR].AsString(""),
        args[VEXPR].AsString(""),
        args[Y].AsInt(3),
        args[U].AsInt(2),
        args[V].AsInt(2),
        args[CHROMA].AsString(""),
        args[LSB].AsBool(false),
        EXPR_X,
        env);
}

AVSValue __cdecl create_exprxy(AVSValue args, void*, IScriptEnvironment* env)
{
    enum { CLIP, CLIP2, EXPR, YEXPR, UEXPR, VEXPR, Y, U, V, CHROMA, LSB };
    return new ClExpr(
        args[CLIP].AsClip(),
        args[CLIP2].AsClip(),
        nullptr,
        args[EXPR].AsString(""),
        args[YEXPR].AsString(""),
        args[UEXPR].AsString(""),
        args[VEXPR].AsString(""),
        args[Y].AsInt(3),
        args[U].AsInt(2),
        args[V].AsInt(2),
        args[CHROMA].AsString(""),
        args[LSB].AsBool(false),
        EXPR_XY,
        env);
}

AVSValue __cdecl create_exprxyz(AVSValue args, void*, IScriptEnvironment* env)
{
    enum { CLIP, CLIP2, CLIP3, EXPR, YEXPR, UEXPR, VEXPR, Y, U, V, CHROMA, LSB };
    return new ClExpr(args[CLIP].AsClip(),
        args[CLIP2].AsClip(),
        args[CLIP3].AsClip(),
        args[EXPR].AsString(""),
        args[YEXPR].AsString(""),
        args[UEXPR].AsString(""),
        args[VEXPR].AsString(""),
        args[Y].AsInt(3),
        args[U].AsInt(2),
        args[V].AsInt(2),
        args[CHROMA].AsString(""),
        args[LSB].AsBool(false),
        EXPR_XYZ,
        env);
}

const AVS_Linkage *AVS_linkage = nullptr;

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors)
{
    AVS_linkage = vectors;

    env->AddFunction("cl_expr", "c[expr]s[yExpr]s[uExpr]s[vExpr]s[Y]i[U]i[V]i[chroma]s[lsb]b", create_expr, 0);
    env->AddFunction("cl_exprxy", "cc[expr]s[yExpr]s[uExpr]s[vExpr]s[Y]i[U]i[V]i[chroma]s[lsb]b", create_exprxy, 0);
    env->AddFunction("cl_exprxyz", "ccc[expr]s[yExpr]s[uExpr]s[vExpr]s[Y]i[U]i[V]i[chroma]s[lsb]b", create_exprxyz, 0);
    return "I'd blame NVIDIA if I were you";
}
