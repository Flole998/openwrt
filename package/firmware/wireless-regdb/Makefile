include $(TOPDIR)/rules.mk

PKG_NAME:=wireless-regdb
PKG_VERSION:=2019-03-01

PKG_SOURCE:=$(PKG_NAME)-master-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://git.kernel.org/pub/scm/linux/kernel/git/sforshee/wireless-regdb.git/snapshot/
PKG_HASH:=c5b31d8bd9771119d585bb5566f1482a43f8cbfa1aeb8ce0b30b7607fcf2e19d

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-master-$(PKG_VERSION)

PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>

include $(INCLUDE_DIR)/package.mk

define Package/wireless-regdb
	SECTION:=firmware
	CATEGORY:=Firmware
	URL:=https://git.kernel.org/pub/scm/linux/kernel/git/sforshee/wireless-regdb.git/
	TITLE:=Wireless Regulatory Database
endef

define Build/Compile
	python $(PKG_BUILD_DIR)/db2fw.py $(PKG_BUILD_DIR)/regulatory.db $(PKG_BUILD_DIR)/db.txt
endef

define Package/wireless-regdb/install
	$(INSTALL_DIR) $(1)/lib/firmware
	$(CP) $(PKG_BUILD_DIR)/regulatory.db $(1)/lib/firmware/
endef

$(eval $(call BuildPackage,wireless-regdb))
