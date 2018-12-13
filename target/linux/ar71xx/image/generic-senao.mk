define Build/senao-factory-image
	mkdir -p $@.senao

	touch $@.senao/before-upgrade.sh
	touch $@.senao/FWINFO-$(BOARDNAME)-OpenWrt-v9.9.9-$(REVISION).txt
	$(CP) $(IMAGE_KERNEL) $@.senao/openwrt-senao-$(1)-uImage-lzma.bin
	$(CP) $@ $@.senao/openwrt-senao-$(1)-root.squashfs

	$(TAR) -c \
		--numeric-owner --owner=0 --group=0 --sort=name \
		$(if $(SOURCE_DATE_EPOCH),--mtime="@$(SOURCE_DATE_EPOCH)") \
		-C $@.senao . | gzip -9nc > $@

	rm -rf $@.senao
endef


define Device/ews860ap
  DEVICE_TITLE := Engenius EWS860AP
  DEVICE_PACKAGES := kmod-ath10k ath10k-firmware-qca988x
  BOARDNAME = EWS860AP
  IMAGE_SIZE = 13120k
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env),320k(custom)ro,13120k(firmware),2560k(failsafe)ro,64k(ART)ro,13120k@0xa0000(firmware)
  IMAGES += factory.bin
  IMAGE/factory.bin := append-rootfs | pad-rootfs | senao-factory-image ews860ap
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += ews860ap

define Device/enh1750ext
  DEVICE_TITLE := Engenius ENH1750EXT
  DEVICE_PACKAGES := kmod-ath10k ath10k-firmware-qca988x
  BOARDNAME = ENH1750EXT
  IMAGE_SIZE = 13120k
  MTDPARTS = spi0.0:256k(u-boot)ro,64k(u-boot-env),320k(custom)ro,13120k(firmware),2560k(failsafe)ro,64k(ART)ro,13120k@0xa0000(firmware)
  IMAGES += factory.bin
  IMAGE/factory.bin := append-rootfs | pad-rootfs | senao-factory-image enh1750ext
  IMAGE/sysupgrade.bin := append-kernel | append-rootfs | pad-rootfs | check-size $$$$(IMAGE_SIZE)
endef
TARGET_DEVICES += enh1750ext
