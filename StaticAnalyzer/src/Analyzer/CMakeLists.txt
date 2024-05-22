
MESSAGE(STATUS "Model This is SOURCE dir " ${PROJECT_SOURCE_DIR})

ADD_LIBRARY(Analyzer ActivityAnalyzer.cpp
                     NuXmv_ActivityAnalyzer.cpp
                     NFAs_ActivityAnalyzer.cpp
                     WPOTrPDS_ActivityAnalyzer.cpp
                     )


