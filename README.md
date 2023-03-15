# Libkvmi

![Build](https://github.com/bitdefender/libkvmi/workflows/Build/badge.svg)

(c) 2017-2023 Bitdefender SRL

## Usage

To test the library, issue:
```
$ ./bootstrap
$ ./configure
$ make
```
This will build the library and the test under `examples/`.

To see the test in action, ensure you have a Linux kernel built with
CONFIG_KVM_INTROSPECTION and already installed
([kvmi-v7 branch](https://github.com/KVM-VMI/kvm/tree/kvmi-v7)).

	Virtualization
		Kernel-based Virtual Machine (KVM) support
		KVM Introspection

You also need QEMU built with VM introspection support
([kvmi-v7 branch](https://github.com/KVM-VMI/qemu/tree/kvmi-v7)).

In the `examples/` subdirectory run:
```
# ./hookguest-libkvmi /tmp/introspector
```
then simply start a KVM domain up with:
```
	qemu-system-x86_64 ... \
		-enable-kvm \
		-chardev socket,path=/tmp/introspector,id=chardev0,reconnect=10 \
		-object introspection,id=kvmi,chardev=chardev0
```

The application can be shut down at any time via `^C`.
