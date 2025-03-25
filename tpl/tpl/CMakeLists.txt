add_library(Util)
target_sources(Util PUBLIC FILE_SET CXX_MODULES FILES util.cpp)

add_library(Tokenizer)
target_sources(Tokenizer PUBLIC FILE_SET CXX_MODULES FILES tokenizer.cpp)

add_library(Parser)
target_sources(Parser PUBLIC FILE_SET CXX_MODULES FILES parser.cpp)
target_link_libraries(Parser PRIVATE Tokenizer Util)

add_library(Runtime)
target_sources(Runtime PUBLIC FILE_SET CXX_MODULES FILES interpreter.cpp value.cpp)
target_link_libraries(Runtime PRIVATE Parser Util)

add_executable(tpl)
target_sources(tpl PUBLIC FILE_SET CXX_MODULES FILES main.cpp)
target_link_libraries(tpl PRIVATE Tokenizer Parser Runtime Util)