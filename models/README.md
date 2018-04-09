Models to be automatically included in ToothMaker after compile can be placed here. One folder per model, with following subfolders:

/data:  Contains the interface XML and associated parameters, images, etc.

/bin:   Model binaries/scripts (if applicable).

/src:   Model source codes (if applicable).

`copy_resources.sh` will automatically copy /data and /bin folder contents for binary models, while /src will be ignored (it can be used simply as a repository for the source).

For library models the model root folder must contain a .pro file, with sources in /src folder. The model .pro files must be listed in `models.pro`.

* Important: Model sources must be accompanied with a license file, or alternatively a licence notification at the beginning of each source file.
