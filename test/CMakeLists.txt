Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.3.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(test 
  test-single-dispatch.cpp
  test-double-dispatch.cpp
  example-regex.cpp)
  
target_link_libraries(test PRIVATE Catch2::Catch2WithMain Josa::Visitor)
target_compile_features(test PRIVATE cxx_std_17)
