# 版本限定 
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)                       
# 项目名称 
set(PROJECT_NAME TokenizerTest)
PROJECT(${PROJECT_NAME})
# 设置编译器选项：O3优化，显示所有警告
# SET(CMAKE_C_FLAGS "$ENV{CFLAGS} -O3 -Wall") 
# 定义头文件的路径变量
#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "release")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
SET(INCLUDE_PATH ${PROJECT_SOURCE_DIR}/paddlenlp)
MESSAGE(STATUS "Include Path, ${INCLUDE_PATH}")
# 定义源文件路径变量
SET(SOURCE_PATH ${PROJECT_SOURCE_DIR}/src) 
MESSAGE(STATUS "Source Path , ${SOURCE_PATH}") 
# 包含头文件路径
INCLUDE_DIRECTORIES(${INCLUDE_PATH} $ENV{PYTHON_INCLUDE_DIRS})
#include_directories("/mnt/zhangxuefei/anaconda3/envs/py3.7/include/python3.7m")

# 添加源文件路径下所有源文件存放到变量中(*.c && *.cpp)，当然也可以手动一个个文件添加进来
AUX_SOURCE_DIRECTORY(${SOURCE_PATH} SRC_LIST) 

# 设置动态库输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) 
MESSAGE(STATUS "Library Output Path, " $ENV{PROJECT_BINARY_DIR}/lib}) 
# 生成动态库(libmymath.so)
ADD_LIBRARY(tokenizer SHARED ${SRC_LIST}) 

# 添加源文件列表变量
# SET(SRC_LIST ${PROJECT_SOURCE_DIR}/src/tokenizer.cpp)
# 打印编译目录和项目目录路径
MESSAGE(STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})

# set(MY_PYBIND ${PROJECT_SOURCE_DIR}/third_party/pybind11-2.5.0)
# add_subdirectory(${MY_PYBIND})
find_package(pybind11)
pybind11_add_module(${PROJECT_NAME} ${SOURCE_PATH}/tokenizer.cc)

# 生成可执行文件
# ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
SET(LINK_DIR /usr/local/lib)
MESSAGE(STATUS "Link Path, ${LINK_DIR}")
LINK_DIRECTORIES(${LINK_DIR})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE utf8proc)
# TARGET_LINK_LIBRARIES(${PROJECT_NAME} public example)
# # 生成静态库（libmymath.a，target名字只能有一个，所以不能与动态库的名字一样）
# ADD_LIBRARY(mymath_static STATIC ${SRC_LIST})
# # 但是可以通过下面的命令更改静态库target生成的库名，这样就和动态库的名字一样的了
# SET_TARGET_PROPERTIES(mymath_static PROPERTIES OUTPUT_NAME "mymath") 
