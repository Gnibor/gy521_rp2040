BUILD_DIR=build

default:
	$(MAKE) -C $(BUILD_DIR)

all:
	cp -f $(PICO_SDK_PATH)/external/pico_sdk_import.cmake .
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make
	cp  build/compile_commands.json ./

flash:
	picotool load $(BUILD_DIR)/gy521_rp2040.uf2

clean:
	rm -rf $(BUILD_DIR)

.PHONY: default all flash clean
