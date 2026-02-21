BUILD_DIR=build

default:
	$(MAKE) -C $(BUILD_DIR)

all:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && make

flash:
	picotool load $(BUILD_DIR)/gy521_rp2040.uf2

clean:
	rm -rf $(BUILD_DIR)

.PHONY: default all flash clean
