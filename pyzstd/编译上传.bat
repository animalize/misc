E:
cd E:\dev\pyzstd

echo Y | rd /q /s build
echo Y | rd /q /s dist

py -3.7-32 setup.py sdist bdist_wheel
py -3.7-64 setup.py sdist bdist_wheel
py -3.8-32 setup.py sdist bdist_wheel
py -3.8-64 setup.py sdist bdist_wheel
py -3.9-32 setup.py sdist bdist_wheel
py -3.9-64 setup.py sdist bdist_wheel

twine upload dist/*

pause