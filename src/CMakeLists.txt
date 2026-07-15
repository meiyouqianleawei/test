# 添加核心模块
add_subdirectory(core)
add_subdirectory(compression)
add_subdirectory(encryption)
add_subdirectory(filter)
add_subdirectory(pack)
add_subdirectory(engine)  # 备份引擎和还原引擎
add_subdirectory(scheduler)  # 定时调度器
add_subdirectory(realtime)  # 实时备份监控

# GUI 模块（可选）
if(BUILD_GUI)
    add_subdirectory(ui)
    message(STATUS "GUI module enabled")
else()
    message(STATUS "GUI module disabled")
endif()