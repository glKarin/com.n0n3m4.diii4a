cmake_minimum_required(VERSION 3.14)
project(fwgs_library_suffix LANGUAGES C)

add_library(fwgs_library_suffix STATIC "src/library_suffix.c")
target_include_directories(fwgs_library_suffix PUBLIC "include")
