#pragma once
enum LayerConnectionType
{
    LAYER_CONNECTION_NONE,            //无连接，用于输入层，不需要特殊设置
    LAYER_CONNECTION_FULLCONNECT,     //全连接
    LAYER_CONNECTION_CONVOLUTION,     //卷积
    LAYER_CONNECTION_POOLING,         //池化
    LAYER_CONNECTION_DIRECT,          //直连
    LAYER_CONNECTION_CORRELATION,     //相关
    LAYER_CONNECTION_COMBINE,         //合并
    LAYER_CONNECTION_EXTRACT,         //抽取
    LAYER_CONNECTION_ROTATE_EIGEN,    //旋转
    LAYER_CONNECTION_NORM2,           //求出每组数据的模
    LAYER_CONNECTION_TRANSPOSE,       //NCHW2NHWC
    LAYER_CONNECTION_NAC,             //NAC
};