project('gnome-qr', 'c', version: '51.0')
gnome = import('gnome')
pkg = import('pkgconfig')
cc = meson.get_compiler('c')
glib_dep = dependency('glib-2.0')
gio_dep = dependency('gio-2.0')
m_dep = cc.find_library('m')
qrcodegen_dep = subproject('qrcodegen').get_variable('qrcodegen_dep')
libargs = ['-DG_LOG_DOMAIN="GnomeDesktop"']
qr_libversion = '0.0.1'
qr_soversion = '0'
configure_file(output: 'config.h', configuration: configuration_data())
add_project_arguments('-I' + meson.project_build_root(), language: 'c')
subdir('libgnome-desktop/gnome-qr')
