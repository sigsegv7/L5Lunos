INTERNAL_CFLAGS = -T$(LDSCRIPT) \
				  -L$(LIBC_DIR) -I$(LIBC_DIR)/include/ \
				  -I$(LIBC_DIR)/$(TARGET)/include/ -pie -no-pie
