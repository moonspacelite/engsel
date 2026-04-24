include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-engsel
PKG_VERSION:=1.0.0
PKG_RELEASE:=1
PKG_MAINTAINER:=0x
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk

define Package/luci-app-engsel
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=LuCI Engsel - Antarmuka Web untuk Engsel CLI
  URL:=https://github.com/moonspacelite/0xygen
  DEPENDS:=+engsel +luci-base +openssl-util +curl +coreutils-base64
  PKGARCH:=all
endef

define Package/luci-app-engsel/description
  Antarmuka web (LuCI) untuk Engsel CLI.
  Semua fitur CLI tersedia di antarmuka grafis:
  login OTP, lihat paket, beli paket, riwayat transaksi,
  fitur lanjutan (Circle, Family Plan, Store, Redeem), dan lainnya.
endef

define Build/Compile
endef

define Package/luci-app-engsel/install
	# --- Lua controller & view (OpenWrt 24 & 25) ---
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/controller
	$(INSTALL_DATA) ./luasrc/controller/engsel.lua \
		$(1)/usr/lib/lua/luci/controller/engsel.lua

	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/view/engsel
	$(INSTALL_DATA) ./luasrc/view/engsel/main.htm \
		$(1)/usr/lib/lua/luci/view/engsel/main.htm

	# --- Static assets (CSS) ---
	$(INSTALL_DIR) $(1)/www/luci-static/engsel
	$(INSTALL_DATA) ./htdocs/luci-static/engsel/engsel.css \
		$(1)/www/luci-static/engsel/engsel.css

	# --- Shell backend ---
	$(INSTALL_DIR) $(1)/usr/libexec
	$(INSTALL_BIN) ./root/usr/libexec/engsel-luci \
		$(1)/usr/libexec/engsel-luci

	# --- rpcd ACL ---
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./root/usr/share/rpcd/acl.d/luci-app-engsel.json \
		$(1)/usr/share/rpcd/acl.d/luci-app-engsel.json
endef

define Package/luci-app-engsel/postinst
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] || {
	rm -f /tmp/luci-indexcache /tmp/luci-modulecache/*
	# OpenWrt 25.x: luci-base may look under /usr/share/luci/ instead of
	# /usr/lib/lua/luci/.  Create compatibility symlinks when that tree exists.
	if [ -d /usr/share/luci/menu.d ] && [ ! -e /usr/share/luci/controller/engsel.lua ]; then
		mkdir -p /usr/share/luci/controller
		ln -sf /usr/lib/lua/luci/controller/engsel.lua \
			/usr/share/luci/controller/engsel.lua 2>/dev/null
		mkdir -p /usr/share/luci/view/engsel
		ln -sf /usr/lib/lua/luci/view/engsel/main.htm \
			/usr/share/luci/view/engsel/main.htm 2>/dev/null
	fi
	/etc/init.d/rpcd restart 2>/dev/null
	/etc/init.d/uhttpd restart 2>/dev/null
}
exit 0
endef

define Package/luci-app-engsel/postrm
#!/bin/sh
rm -f /usr/share/luci/controller/engsel.lua 2>/dev/null
rm -rf /usr/share/luci/view/engsel 2>/dev/null
rm -f /tmp/luci-indexcache /tmp/luci-modulecache/*
exit 0
endef

$(eval $(call BuildPackage,luci-app-engsel))
