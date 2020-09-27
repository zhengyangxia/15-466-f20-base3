.PHONY : all

#n.b. the '-y' sets autoexec scripts to 'on' so that driver expressions will work
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	BLENDER = /Applications/Blender/blender.app/Contents/MacOS/blender -y
else ifeq ($(UNAME_S),Linux)
	BLENDER = ../../blender-2.90.0-linux64/blender -y
else
	BLENDER = blender
endif

EXPORT_MESHES=export-meshes.py
EXPORT_SCENE=export-scene.py

DIST=../dist

FILE_NAME=maze

all : \
	$(DIST)/$(FILE_NAME).pnct \
	$(DIST)/$(FILE_NAME).scene \


$(DIST)/$(FILE_NAME).scene : $(FILE_NAME).blend $(EXPORT_SCENE)
	$(BLENDER) --background --python $(EXPORT_SCENE) -- '$<':Main '$@'

$(DIST)/$(FILE_NAME).pnct : $(FILE_NAME).blend $(EXPORT_MESHES)
	$(BLENDER) --background --python $(EXPORT_MESHES) -- '$<':Main '$@'
