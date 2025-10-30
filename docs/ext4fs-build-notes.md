# Building the ext4fs Server

The `userspace/servers/ext4fs` program depends on the `libext2fs` and `libcom_err`
libraries that ship with the `e2fsprogs` project. On Debian-based systems these
come from the `libext2fs-dev` and `comerr-dev` packages.

When building inside the automated evaluation container we currently cannot fetch
packages from the Ubuntu mirrors because outbound HTTP requests are blocked by a
proxy that returns HTTP 403 responses. As a result, `apt-get update` and
subsequent `apt-get install` commands fail, preventing the development packages
from being installed.

To build `ext4fs` outside of this restricted environment, run:

```sh
sudo apt-get update
sudo apt-get install libext2fs-dev comerr-dev
make -C userspace servers/ext4fs/ext4fs
```

If you need to build inside the container, you will have to provide the required
libraries manually (for example by vendoring prebuilt `.deb` packages or by
copying the headers and static libraries into the repository) before running the
`make` command above.
