#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# small3dlib and tinyphysicsengine demos: one .rock per demo.

S3L_SRCDIR := $(APPSDIR)/plugins/s3l
S3L_BUILDDIR := $(BUILDDIR)/apps/plugins/s3l
S3L_SRC := $(call preprocess, $(S3L_SRCDIR)/SOURCES)
S3L_OBJ := $(call c2obj, $(S3L_SRC))
OTHER_SRC += $(S3L_SRC)
ROCKS += $(S3L_SRC:$(S3L_SRCDIR)/%.c=$(S3L_BUILDDIR)/%.rock)

$(foreach c,$(notdir $(S3L_SRC:.c=)),$(eval $(S3L_BUILDDIR)/$(c).rock: $(S3L_BUILDDIR)/$(c).o))
