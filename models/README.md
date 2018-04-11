Models to be automatically included in ToothMaker after compile can be placed here. One folder per model with following subfolders:

* data:  Contains the interface XML and associated parameters, images, etc.

* bin:   Model binaries/scripts.

* src:   Model source codes (if applicable).

Calling `make resources` executes `copy_resources.sh` that copies /data and /bin folder contents to ToothMaker resources folder, while /src will be ignored (it can be used as a repository for source codes).

Important: Model sources must be accompanied with a license file, or alternatively a licence notification at the beginning of each source file.
