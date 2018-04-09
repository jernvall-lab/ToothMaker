# Mac OS X 10.6 (Snow Leopard)

Install Qt 5.1.1, which is the latest Qt version supported. You'll also need a GCC with C++11 support for building and updated cctools for running `macdeployqt`. MacPorts is the recommended method:

```shell
sudo port install gcc6 cctools
```

After installing GCC, enter the build folder and type

```shell
qmake ../ToothMaker.pro "OSX=10.6";
make;
make resources
```

Then call

```shell
macdeployqt interface/ToothMaker.app
```

to make a self-contained bundle. However, this will return with some errors and only partially do the job. To finish the process, call

```shell
sh ../OSX106/install_names_toothmaker.sh interface/ToothMaker.app
```

Note that `install_names_toothmaker.sh` expect a MacPorts installation at the default location (/opt).
