cmake_minimum_required(VERSION 3.10.0)
add_custom_target(docs doxygen api_guide.cfg
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(docs_clean rm docs/* -rf
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
