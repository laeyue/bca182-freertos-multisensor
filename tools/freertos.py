"""Build the native FreeRTOS kernel shipped with STM32CubeF1."""

import os

Import("env")  # noqa: F821 - injected by PlatformIO/SCons

framework = env.PioPlatform().get_package_dir("framework-stm32cubef1")
source = os.path.join(framework, "Middlewares", "Third_Party", "FreeRTOS", "Source")
env.Append(CPPPATH=[
    os.path.join(source, "include"),
    os.path.join(source, "portable", "GCC", "ARM_CM3"),
])
env.BuildSources(
    os.path.join("$BUILD_DIR", "freertos-core"),
    source,
    src_filter=[
        "+<tasks.c>", "+<queue.c>", "+<list.c>",
        "+<event_groups.c>", "+<timers.c>",
    ],
)
env.BuildSources(
    os.path.join("$BUILD_DIR", "freertos-port"),
    os.path.join(source, "portable", "GCC", "ARM_CM3"),
    src_filter=["+<port.c>"],
)
env.BuildSources(
    os.path.join("$BUILD_DIR", "freertos-heap"),
    os.path.join(source, "portable", "MemMang"),
    src_filter=["+<heap_4.c>"],
)

