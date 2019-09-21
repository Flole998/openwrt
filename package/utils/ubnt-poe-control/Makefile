include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=ubnt-poe-control
PKG_VERSION:=1.0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/ubnt-poe-control
  SUBMENU:=Device drivers
  TITLE:=Ubiquiti POE Switch driver (POE control)
  FILES:=$(PKG_BUILD_DIR)/ubnt-poe-control.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoLoad,61,ubnt-poe-control)
endef

define KernelPackage/ubnt-poe-control/description
 Ubiquiti POE Switch driver (POE control)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	cp src/Makefile src/ubnt-poe-control.c $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C $(LINUX_DIR) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)" \
		KERNELDIR=$(LINUX_DIR) \
		CC="$(TARGET_CC)" \
		modules
endef

$(eval $(call KernelPackage,ubnt-poe-control))
