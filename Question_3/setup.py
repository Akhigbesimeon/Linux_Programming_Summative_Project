from setuptools import setup, Extension

vibration_module = Extension(
    'vibration',            
    sources=['vibration.c'],
    extra_compile_args=['-O3'] # Enable aggressive compiler optimizations
)

setup(
    name='vibration_analytics',
    version='1.0',
    description='High-performance C extension for industrial vibration analysis',
    ext_modules=[vibration_module]
)
