target_sources(${APP_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppDefine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppDefine.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppPal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppPal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppTextureManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LAppTextureManager.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mainMinimum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismUserModelExtend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismUserModelExtend.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MouseActionManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MouseActionManager.hpp
  )

target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
