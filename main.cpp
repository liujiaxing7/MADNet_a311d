/****************************************************************************
*   Generated by ACUITY 6.0.12
*   Match ovxlib 1.1.34
*
*   Neural Network application project entry file
****************************************************************************/
/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <time.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <opencv2/opencv.hpp> // opencv
#elif defined(_WIN32)
#include <windows.h>
#endif

#define _BASETSD_H
#define INPUT_META_NUM 1
#include "vsi_nn_pub.h"

#include "vnn_global.h"
#include "vnn_pre_process.h"
#include "vnn_post_process.h"
#include "vnn_model.h"

const std::string SUFFIX_IMAGE = "png bmp tiff tif jpg jpeg PNG JPG JPEG";
static vnn_input_meta_t input_meta_tab[INPUT_META_NUM];
#define RELEASE(x) {if(nullptr != (x)) free((x)); (x) = nullptr;}

/*-------------------------------------------
        Macros and Variables
-------------------------------------------*/

/*-------------------------------------------
                  Functions
-------------------------------------------*/
static void vnn_ReleaseNeuralNetwork
    (
    vsi_nn_graph_t *graph
    )
{
    vnn_ReleaseModel( graph, TRUE );
    if (vnn_UseImagePreprocessNode())
    {
        vnn_ReleaseBufferImage();
    }
}

static vsi_status vnn_PostProcessNeuralNetwork
    (
    vsi_nn_graph_t *graph
    )
{
    return vnn_PostProcessModel( graph );
}

#define BILLION                                 1000000000
static uint64_t get_perf_count()
{
#if defined(__linux__) || defined(__ANDROID__) || defined(__QNX__) || defined(__CYGWIN__)
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)((uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * BILLION);
#elif defined(_WIN32) || defined(UNDER_CE)
    LARGE_INTEGER ln;

    QueryPerformanceCounter(&ln);

    return (uint64_t)ln.QuadPart;
#endif
}

static vsi_status vnn_VerifyGraph
    (
    vsi_nn_graph_t *graph
    )
{
    vsi_status status = VSI_FAILURE;
    uint64_t tmsStart, tmsEnd, msVal, usVal;

    /* Verify graph */
    printf("Verify...\n");
    tmsStart = get_perf_count();
    status = vsi_nn_VerifyGraph( graph );
    TEST_CHECK_STATUS(status, final);
    tmsEnd = get_perf_count();
    msVal = (tmsEnd - tmsStart)/1000000;
    usVal = (tmsEnd - tmsStart)/1000;
    printf("Verify Graph: %ldms or %ldus\n", msVal, usVal);

final:
    return status;
}

static vsi_status vnn_ProcessGraph
    (
    vsi_nn_graph_t *graph
    )
{
    vsi_status status = VSI_FAILURE;
    int32_t i,loop;
    char *loop_s;
    uint64_t tmsStart, tmsEnd, sigStart, sigEnd;
    float msVal, usVal;

    status = VSI_FAILURE;
    loop = 1; /* default loop time is 1 */
    loop_s = getenv("VNN_LOOP_TIME");
    if(loop_s)
    {
        loop = atoi(loop_s);
    }

    /* Run graph */
    tmsStart = get_perf_count();
    printf("Start run graph [%d] times...\n", loop);
    for(i = 0; i < loop; i++)
    {
        sigStart = get_perf_count();
#ifdef VNN_APP_ASYNC_RUN
        status = vsi_nn_AsyncRunGraph( graph );
        if(status != VSI_SUCCESS)
        {
            printf("Async Run graph the %d time fail\n", i);
        }
        TEST_CHECK_STATUS( status, final );

        //do something here...

        status = vsi_nn_AsyncRunWait( graph );
        if(status != VSI_SUCCESS)
        {
            printf("Wait graph the %d time fail\n", i);
        }
#else
        status = vsi_nn_RunGraph( graph );
        if(status != VSI_SUCCESS)
        {
            printf("Run graph the %d time fail\n", i);
        }
#endif
        TEST_CHECK_STATUS( status, final );

        sigEnd = get_perf_count();
        msVal = (sigEnd - sigStart)/1000000;
        usVal = (sigEnd - sigStart)/1000;
        printf("Run the %u time: %.2fms or %.2fus\n", (i + 1), msVal, usVal);
    }
    tmsEnd = get_perf_count();
    msVal = (tmsEnd - tmsStart)/1000000;
    usVal = (tmsEnd - tmsStart)/1000;
    printf("vxProcessGraph execution time:\n");
    printf("Total   %.2fms or %.2fus\n", msVal, usVal);
    printf("Average %.2fms or %.2fus\n", ((float)usVal)/1000/loop, ((float)usVal)/loop);

final:
    return status;
}

static vsi_status vnn_PreProcessNeuralNetwork
    (
    vsi_nn_graph_t *graph,
    int argc,
    char **argv
    )
{
    /*
     * argv0:   execute file
     * argv1:   data file
     * argv2~n: inputs n file
     */
    const char **inputs = (const char **)argv + 2;
    uint32_t input_num = argc - 2;

    /*
    if(vnn_UseImagePreprocessNode())
    {
        return vnn_PreProcessModel_ImageProcess(graph, inputs, input_num);
    }
    */
    return vnn_PreProcessModel( graph, inputs, input_num );
}

static vsi_nn_graph_t *vnn_CreateNeuralNetwork
    (
    char *data_file_name
    )
{
    vsi_nn_graph_t *graph = NULL;
    uint64_t tmsStart, tmsEnd, msVal, usVal;

    tmsStart = get_perf_count();
    graph = vnn_CreateModel( data_file_name, NULL,
                      vnn_GetPreProcessMap(), vnn_GetPreProcessMapCount(),
                      vnn_GetPostProcessMap(), vnn_GetPostProcessMapCount() );
    TEST_CHECK_PTR(graph, final);
    tmsEnd = get_perf_count();
    msVal = (tmsEnd - tmsStart)/1000000;
    usVal = (tmsEnd - tmsStart)/1000;
    printf("Create Neural Network: %ldms or %ldus\n", msVal, usVal);

final:
    return graph;
}

std::string GetSuffix(const char *fileName)
{
    const char SEPARATOR = '.';
    char buff[32] = {0};

    const char *ptr = strrchr(fileName, SEPARATOR);

    if (nullptr == ptr) return "";

    uint32_t pos = ptr - fileName;
    uint32_t n = strlen(fileName) - (pos + 1);
    strncpy(buff, fileName + (pos + 1), n);

    return buff;
}

bool IsIMAGE(const char *fileName)
{
    std::string suffix = GetSuffix(fileName);

    if (suffix.empty()) return false;
    return SUFFIX_IMAGE.find(suffix) != std::string::npos;
}

void Walk(const std::string &path, const std::string suffixList
        , std::vector<std::string> &fileList)
{
    DIR *dir;
    dir = opendir(path.c_str());
    struct dirent *ent;
    if (nullptr == dir)
    {
//        std::cout << "failed to open file " << path << std::endl;
        return;
    }

    while ((ent = readdir(dir)) != nullptr)
    {
        auto name = std::string(ent->d_name);

        // ignore "." ".."
        if (name.size() < 4) continue;

        std::string suffix = GetSuffix(name.c_str());

        if (!suffix.empty() && suffixList.find(suffix) != std::string::npos)
        {
            fileList.emplace_back(path + "/" + name);
        }
        else
        {
            Walk(path + "/" + name, suffixList, fileList);
        }

    }

    closedir(dir);
}
static void _load_input_meta()
{
    uint32_t i;
    for (i = 0; i < INPUT_META_NUM; i++)
    {
        memset(&input_meta_tab[i].image.preprocess,
               VNN_PREPRO_NONE, sizeof(int32_t) * VNN_PREPRO_NUM);
    }
    /* lid: left_485 */
    input_meta_tab[0].image.preprocess[0] = VNN_PREPRO_REORDER;
    input_meta_tab[0].image.preprocess[1] = VNN_PREPRO_MEAN;
    input_meta_tab[0].image.preprocess[2] = VNN_PREPRO_SCALE;
    input_meta_tab[0].image.reorder[0] = 2;
    input_meta_tab[0].image.reorder[1] = 1;
    input_meta_tab[0].image.reorder[2] = 0;
    input_meta_tab[0].image.mean[0] = 128.0;
    input_meta_tab[0].image.mean[1] = 128.0;
    input_meta_tab[0].image.mean[2] = 128.0;
    input_meta_tab[0].image.scale = 0.00715;

    /* lid: right_486 */
    input_meta_tab[1].image.preprocess[0] = VNN_PREPRO_REORDER;
    input_meta_tab[1].image.preprocess[1] = VNN_PREPRO_MEAN;
    input_meta_tab[1].image.preprocess[2] = VNN_PREPRO_SCALE;
    input_meta_tab[1].image.reorder[0] = 2;
    input_meta_tab[1].image.reorder[1] = 1;
    input_meta_tab[1].image.reorder[2] = 0;
    input_meta_tab[1].image.mean[0] = 128.0;
    input_meta_tab[1].image.mean[1] = 128.0;
    input_meta_tab[1].image.mean[2] = 128.0;
    input_meta_tab[1].image.scale = 0.00715;


}
static float *_imageData_to_float32(uint8_t *bmpData, vsi_nn_tensor_t *tensor)
{
    float *fdata;
    uint32_t sz, i;

    fdata = nullptr;
    sz = vsi_nn_GetElementNum(tensor);
    fdata = (float *) malloc(sz * sizeof(float));
    TEST_CHECK_PTR(fdata, final);

    for (i = 0; i < sz; i++)
    {
        fdata[i] = (float) bmpData[i];
    }

    final:
    return fdata;
}

static void _data_transform(float *fdata, vnn_input_meta_t *meta, vsi_nn_tensor_t *tensor)
{
    uint32_t s0, s1, s2;
    uint32_t i, j, offset, sz, order;
    float *data;
    uint32_t *reorder;

    data = nullptr;
    reorder = meta->image.reorder;
    s0 = tensor->attr.size[0];
    s1 = tensor->attr.size[1];
    s2 = tensor->attr.size[2];
    sz = vsi_nn_GetElementNum(tensor);
    data = (float *) malloc(sz * sizeof(float));
    TEST_CHECK_PTR(data, final);
    memset(data, 0, sizeof(float) * sz);

    for (i = 0; i < s2; i++)
    {
        if (s2 > 1 && reorder[i] <= s2)
        {
            order = reorder[i];
        }
        else
        {
            order = i;
        }

        offset = s0 * s1 * i;
        for (j = 0; j < s0 * s1; j++)
        {
            data[j + offset] = fdata[j * s2 + order];
        }
    }


    memcpy(fdata, data, sz * sizeof(float));
    final:
    if (data)free(data);
}

static void _data_mean(float *fdata, vnn_input_meta_t *meta, vsi_nn_tensor_t *tensor)
{
    uint32_t s0, s1, s2;
    uint32_t i, j, offset;
    float val, mean;

    s0 = tensor->attr.size[0];
    s1 = tensor->attr.size[1];
    s2 = tensor->attr.size[2];

    for (i = 0; i < s2; i++)
    {
        offset = s0 * s1 * i;
        mean = meta->image.mean[i];
        for (j = 0; j < s0 * s1; j++)
        {
            val = fdata[offset + j] - mean;
            fdata[offset + j] = val;
        }
    }

}

static void _data_scale(float *fdata, vnn_input_meta_t *meta, vsi_nn_tensor_t *tensor)
{
    uint32_t i, sz;
    float val, scale;

    sz = vsi_nn_GetElementNum(tensor);
    scale = meta->image.scale;
    if (0 != scale)
    {
        for (i = 0; i < sz; i++)
        {
            val = fdata[i] * scale;
            fdata[i] = val;
        }
    }
}

static uint8_t *_float32_to_dtype(float *fdata, vsi_nn_tensor_t *tensor)
{
    vsi_status status;
    uint8_t *data;
    uint32_t sz, i, stride;

    sz = vsi_nn_GetElementNum(tensor);
    stride = vsi_nn_TypeGetBytes(tensor->attr.dtype.vx_type);
    data = (uint8_t *) malloc(stride * sz * sizeof(uint8_t));
    TEST_CHECK_PTR(data, final);
    memset(data, 0, stride * sz * sizeof(uint8_t));

    for (i = 0; i < sz; i++)
    {
        status = vsi_nn_Float32ToDtype(fdata[i], &data[stride * i], &tensor->attr.dtype);
        if (status != VSI_SUCCESS)
        {
            if (data)free(data);
            return nullptr;
        }
    }

    final:
    return data;
}
static uint8_t *PreProcess(vsi_nn_tensor_t *tensor, vnn_input_meta_t *meta, uint8_t *bmpData)
{
    uint8_t *data = nullptr;
    float *fdata = nullptr;
    int32_t use_image_process = vnn_UseImagePreprocessNode();

    TEST_CHECK_PTR(bmpData, final);
    if (use_image_process) return bmpData;

    fdata = _imageData_to_float32(bmpData, tensor);
//    PrintMatrix(fdata, 320);
    TEST_CHECK_PTR(fdata, final);

    for (uint32_t i = 0; i < _cnt_of_array(meta->image.preprocess); i++)
    {
        switch (meta->image.preprocess[i])
        {
            case VNN_PREPRO_NONE:
                break;
            case VNN_PREPRO_REORDER:
                _data_transform(fdata, meta, tensor);
                break;
            case VNN_PREPRO_MEAN:
                _data_mean(fdata, meta, tensor);
                break;
            case VNN_PREPRO_SCALE:
                _data_scale(fdata, meta, tensor);
                break;
            default:
                break;
        }
    }

//    PrintMatrix(fdata, 320);
    data = _float32_to_dtype(fdata, tensor);
//    PrintMatrix(data, 320);
    TEST_CHECK_PTR(data, final);
    final:
    RELEASE(fdata);

    return data;
}

static vsi_status _handle_multiple_inputs(vsi_nn_graph_t *graph, uint8_t *data)
{
    const int inputID = 0;
    vsi_status status = VSI_FAILURE;
    vsi_nn_tensor_t *tensor = nullptr;
    char dumpInput[128];

    tensor = vsi_nn_GetTensor(graph, graph->input.tensors[inputID]);
    data = PreProcess(tensor, &(input_meta_tab[0]), data);
    status = vsi_nn_CopyDataToTensor(graph, tensor, data);
    TEST_CHECK_STATUS(status, final);

    status = VSI_SUCCESS;
    final:
    return status;
}
//static vsi_status _handle_multiple_inputs
//        (
//                vsi_nn_graph_t *graph,
//                uint32_t idx,
//                const char *input_file
//        )
//{
//    vsi_status status;
//    vsi_nn_tensor_t *tensor;
//    uint8_t *data;
//    vnn_input_meta_t meta;
//    vsi_enum fileType;
//    char dumpInput[128];
//
//    status = VSI_FAILURE;
//    data = NULL;
//    tensor = NULL;
//    memset(&meta, 0, sizeof(vnn_input_meta_t));
//    tensor = vsi_nn_GetTensor( graph, graph->input.tensors[idx] );
//    meta = input_meta_tab[idx];
//    fileType = _get_file_type(input_file);
//    switch(fileType)
//    {
//        case NN_FILE_JPG:
//            data = _get_jpeg_data(tensor, &meta, input_file);
//            TEST_CHECK_PTR(data, final);
//            break;
//        case NN_FILE_TENSOR:
//            data = _get_tensor_data(tensor, input_file);
//            TEST_CHECK_PTR(data, final);
//            break;
//        case NN_FILE_QTENSOR:
//            data = _get_qtensor_data(tensor, input_file);
//            TEST_CHECK_PTR(data, final);
//            break;
//        case NN_FILE_BINARY:
//            data = _get_binary_data(tensor, input_file);
//            TEST_CHECK_PTR(data, final);
//            break;
//        default:
//            printf("error input file type\n");
//            break;
//    }
//
//    /* Copy the Pre-processed data to input tensor */
//    status = vsi_nn_CopyDataToTensor(graph, tensor, data);
//    TEST_CHECK_STATUS(status, final);
//
//    /* Save the image data to file */
//    snprintf(dumpInput, sizeof(dumpInput), "input_%d.dat", idx);
//    vsi_nn_SaveTensorToBinary(graph, tensor, dumpInput);
//
//
//    status = VSI_SUCCESS;
//    final:
//    if(data)free(data);
//    return status;
//}

vsi_status vnn_PreProcessNeuralNetworkSuperpoint(vsi_nn_graph_t *graph, cv::Mat dst)
{
    cv::Mat dst_gray;
    cv::cvtColor(dst, dst_gray, cv::COLOR_RGB2GRAY);
//    dst_gray /= 255.0f;

    _load_input_meta();
//    using TYPE = uint8_t;
//    TYPE *src = (TYPE *) dst.data;
    vsi_status status = _handle_multiple_inputs(graph, dst_gray.data);
    TEST_CHECK_STATUS(status, final);

    status = VSI_SUCCESS;
    final:
    return status;
}


/*-------------------------------------------
                  Main Functions
-------------------------------------------*/
int main
    (
    int argc,
    char **argv
    )
{
    vsi_status status = VSI_FAILURE;
    vsi_nn_graph_t *graph;
    char *data_name = NULL;

    if(argc < 3)
    {
        printf("Usage: %s data_file inputs...\n", argv[0]);
        return -1;
    }

    data_name = (char *)argv[1];

    std::vector<std::string> imageFiles({});
    char *path = (argv + 2)[0];

    if (IsIMAGE(path))
    {
        imageFiles.emplace_back(path);
    }
    else
    {
        Walk(path, SUFFIX_IMAGE, imageFiles);
    }
    graph = vnn_CreateNeuralNetwork(data_name);
    TEST_CHECK_PTR(graph, final);

    for (const auto &file: imageFiles)
    {
        int width = 640;
        int height = 480;
        int channel = 1;
        cv::Mat dst;
        cv::Mat orig_img = cv::imread(file);
        cv::resize(orig_img, dst, cv::Size(width, height));


    //    /* Create the neural network */
    //    graph = vnn_CreateNeuralNetwork( data_name );
    //    TEST_CHECK_PTR( graph, final );

        /* Pre process the image data */
//        status = vnn_PreProcessNeuralNetwork( graph, argc, argv );
        status = vnn_PreProcessNeuralNetworkSuperpoint(graph, dst);
        TEST_CHECK_STATUS( status, final );

        /* Verify graph */
        status = vnn_VerifyGraph( graph );
        TEST_CHECK_STATUS( status, final);

        /* Process graph */
        status = vnn_ProcessGraph( graph );
        TEST_CHECK_STATUS( status, final );

    //    if(VNN_APP_DEBUG)
    //    {
    //        /* Dump all node outputs */
    //        vsi_nn_DumpGraphNodeOutputs(graph, "./network_dump", NULL, 0, TRUE, 0);
    //    }

        /* Post process output data */
        status = vnn_PostProcessNeuralNetwork( graph );
        TEST_CHECK_STATUS( status, final );
    }

final:
    vnn_ReleaseNeuralNetwork( graph );
    fflush(stdout);
    fflush(stderr);
    return status;
}

