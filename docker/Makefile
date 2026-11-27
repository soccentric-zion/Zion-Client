IMAGE_NAME ?= zion

.PHONY: build run clean delete-images

build:
	@echo "Building Docker image $(IMAGE_NAME) for Zynq UltraScale+ (ARM64)..."
	docker buildx build --platform linux/arm64 --load --build-arg UID=$(shell id -u) --build-arg GID=$(shell id -g) -t $(IMAGE_NAME) .

run:
	# No --gpus flag: Zynq UltraScale+ has no CUDA-capable GPU
	docker run -it --rm --hostname "Zynq UltraScale+" $(IMAGE_NAME)

clean:
	docker rmi $(IMAGE_NAME) || true

delete-images:
	docker rmi $(docker images -q) || true
