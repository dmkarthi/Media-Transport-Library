project('RxApp-X11', 'c', version: '1.0.0')

# Compiler
cc = meson.get_compiler('c')

# Dependencies
mtl_dep = dependency('mtl', required: true)
m_dep = cc.find_library('m', required: true)  
pthread_dep = cc.find_library('pthread', required: true)
x11_dep = dependency('x11', required: true)

# Include directories
inc_dirs = ['/usr/local/include/mtl', 'include']

# Compiler arguments
add_global_arguments('-DALLOW_EXPERIMENTAL_API', language: 'c')

# Executable
executable('RxApp-X11',
  sources: [
    'src/rx_app_main.c',
    'src/frame_converter.c',
    'src/x11_handler.c',
    'src/config_reader.c',
    'src/session_manager.c'
  ],
  include_directories: include_directories('include'),
  dependencies: [mtl_dep, m_dep, pthread_dep, x11_dep],
  c_args: ['-I/usr/local/include/mtl'],
  install: false
)