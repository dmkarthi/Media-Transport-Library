project('TxApp', 'c', version: '1.0.0')

# Compiler
cc = meson.get_compiler('c')

# Dependencies
mtl_dep = dependency('mtl', required: true)
m_dep = cc.find_library('m', required: true)  
pthread_dep = cc.find_library('pthread', required: true)

# Include directories
inc_dirs = ['/usr/local/include/mtl']

# Compiler arguments
add_global_arguments('-DALLOW_EXPERIMENTAL_API', language: 'c')

# Get compiler
cc = meson.get_compiler('c')

# Dependencies
mtl_dep = dependency('mtl', required: true)
libm = cc.find_library('m', required: false)
libpthread = cc.find_library('pthread', required: true)

# Include directories
inc_dirs = include_directories('.')

# Source files
sources = files('src/tx_app_main.c')

# Compiler arguments
c_args = []
c_args += ['-DALLOW_EXPERIMENTAL_API']

# Build executable
# Executable
executable('TxApp',
  sources: [
    'src/tx_app_main.c',
    'src/config_reader.c',
    'src/session_manager.c'
  ],
  include_directories: include_directories('include'),
  dependencies: [mtl_dep, m_dep, pthread_dep],
  c_args: ['-I/usr/local/include/mtl'],
  install: false
)