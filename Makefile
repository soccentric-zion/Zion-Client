# Master Makefile for ACU Full Test Project
# Docker-based builds: the default (and only) build path is Docker.

.DEFAULT_GOAL := help

.PHONY: all build clean test format cppcheck doc help
.PHONY: docker-build docker-configure docker-clean docker-shell docker-install docker-image docker-test
.PHONY: deploy deploy-a deploy-b run-a run-b list setup-completion

# Build directories (separate to avoid CMake cache conflicts)
CROSS_BUILD_DIR  := build/cross

# Parallel jobs (override with make JOBS=N)
JOBS := $(shell nproc)
# QEMU ARM64 emulation can't sustain as many parallel GCC processes as the
# host; cap Docker cross-builds to avoid cc1plus segfaults under heavy load.
DOCKER_JOBS := $(shell echo $$(( $(shell nproc) < 4 ? $(shell nproc) : 4 )))
DOCKER_IMAGE := zion

# Mount the *parent* Zion/ directory so both Zion-Client and Zion-HAL
# are visible inside the container (CMakeLists.txt references ../Zion-HAL/...).
Zion_ROOT := $(shell dirname $(realpath $(CURDIR)))
DOCKER_SRC_DIR         := /workspace/Zion-Client
DOCKER_CROSS_BUILD_DIR := $(DOCKER_SRC_DIR)/build/cross

# Host architecture as shown by file(1) (e.g. x86-64, aarch64)
HOST_ARCH := $(shell uname -m | sed 's/x86_64/x86-64/')

# IP addresses for target devices
IP_A := 192.168.1.3
IP_B := 192.168.1.13

# User/Password for SSH/SCP
USER := $(or $(HAL_SSH_USER),root)
PASS := $(HAL_SSH_PASS)
SSHPASS := $(if $(PASS),sshpass -p $(PASS))
SSH := $(SSHPASS) ssh -o StrictHostKeyChecking=no
SCP := $(SSHPASS) scp -o StrictHostKeyChecking=no

# Default target
all: build ## Docker-based default build

# Deploy to remote target: $(1)=IP address
define deploy_remote
	@echo "Deploying to $(1)..."
	$(SSH) $(USER)@$(1) "rm -rf /tmp/acu-test && mkdir -p /tmp/acu-test"
	$(SCP) -r $(CROSS_BUILD_DIR)/lib $(USER)@$(1):/tmp/acu-test/
	$(SCP) -r $(CROSS_BUILD_DIR)/bin $(USER)@$(1):/tmp/acu-test/
endef

# Run apps on remote target: $(1)=IP address
define run_remote
	@APP="$(word 2, $(MAKECMDGOALS))"; \
	ARGS="$(wordlist 3, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))"; \
	if [ -z "$$APP" ]; then \
		echo "Running all test applications on $(1)..."; \
		for name in $$($(SSH) $(USER)@$(1) "ls /tmp/acu-test/bin/"); do \
			echo "--- Running $$name ---"; \
			$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$name" || true; \
		done; \
	else \
		echo "Running $$APP on $(1) with args: $$ARGS"; \
		$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$APP $$ARGS"; \
	fi
endef

# =============================================================================
# Build & test (Docker — the default build path)
# =============================================================================

build: docker-build ## Build the project in Docker (alias for docker-build)
test: docker-test ## Run tests in Docker (ARM64 via QEMU)

# =============================================================================
# Common for native and Cross
# =============================================================================

clean: ## Remove build directory
	@echo "Cleaning build directory..."
	rm -rf build

list: ## List available test applications
	@echo "Cross ($(CROSS_BUILD_DIR)):"
	@ls -1 $(CROSS_BUILD_DIR)/bin/ 2>/dev/null | sed 's/^/    /' || echo "    (none - run 'make docker-build' first)"

# =============================================================================
# Cross builds (Docker)
# =============================================================================

# -t: allocate a pseudo-TTY so Ctrl-C (SIGINT) is forwarded to the container
# --init: use tini as PID 1 to properly propagate signals to child processes (e.g. ninja)
# ccache volume: persist compilation cache across docker runs
# Zion_ROOT is mounted so ../Zion-HAL/protos and ../Zion-HAL/src resolve inside the container.
DOCKER_RUN := docker run --rm -t --init --platform linux/arm64 -v $(Zion_ROOT):/workspace -v ccache-zion-client:/home/builder/.ccache $(DOCKER_IMAGE) bash -c

docker-install: ## Install Docker if not present
	@if ! command -v docker > /dev/null 2>&1; then \
		echo "Docker not found. Installing Docker..."; \
		sudo apt update && sudo apt install -y docker.io; \
	else \
		echo "Docker is already installed."; \
	fi

docker-image: docker-install ## Build Docker image if not exists
	@if ! docker image inspect $(DOCKER_IMAGE) >/dev/null 2>&1; then \
		echo "Building Docker image $(DOCKER_IMAGE)..."; \
		make -C docker build; \
	else \
		echo "Docker image $(DOCKER_IMAGE) already exists."; \
	fi

docker-configure: docker-image ## Configure project with Docker/Ninja
	@echo "Configuring project with Docker/Ninja..."
	$(DOCKER_RUN) "mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja -DBUILD_TESTS=ON $(DOCKER_SRC_DIR)"

docker-build: docker-image ## Build project with Docker/Ninja
	@echo "Building project with Docker/Ninja..."
	$(DOCKER_RUN) "if [ ! -f $(DOCKER_CROSS_BUILD_DIR)/build.ninja ]; then mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja -DBUILD_TESTS=ON $(DOCKER_SRC_DIR); fi && cd $(DOCKER_CROSS_BUILD_DIR) && ninja -j$(DOCKER_JOBS)"

docker-test: docker-build
	@echo "Running tests in Docker (ARM64 via QEMU)..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ctest --output-on-failure"

docker-clean: docker-image ## Clean Docker build artifacts
	@echo "Cleaning Docker build directory..."
	$(DOCKER_RUN) "rm -rf $(DOCKER_CROSS_BUILD_DIR)"

docker-shell: docker-image ## Start Docker shell for development
	@echo "Starting Docker shell for development..."
	docker run -it --rm --platform linux/arm64 -v $(Zion_ROOT):/workspace -v ccache-zion-client:/home/builder/.ccache $(DOCKER_IMAGE) bash

# =============================================================================
# Deploy & Run (on target device)
# =============================================================================

deploy: deploy-a deploy-b ## Deploy to all devices (deploy-a + deploy-b)

deploy-a: docker-build ## Deploy lib and bin to $(IP_A)
	$(call deploy_remote,$(IP_A))

deploy-b: docker-build ## Deploy lib and bin to $(IP_B)
	$(call deploy_remote,$(IP_B))

run-a: deploy-a ## Run test app on $(IP_A) (usage: make run-a [name] [args...], no args = run all)
	$(call run_remote,$(IP_A))

run-b: deploy-b ## Run test app on $(IP_B) (usage: make run-b [name] [args...], no args = run all)
	$(call run_remote,$(IP_B))

# =============================================================================
# Code quality & Docs (run inside the Docker build container)
# =============================================================================

format: docker-image ## Format code with clang-format (in Docker)
	@echo "Running clang-format in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja format"

cppcheck: docker-image ## Run static analysis with cppcheck (in Docker)
	@echo "Running cppcheck in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja cppcheck"

doc: docker-image ## Generate Doxygen documentation (in Docker)
	@echo "Generating documentation in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja doc_doxygen"

# =============================================================================
# Misc
# =============================================================================

setup-completion:
	@echo "Setting up bash completion for make..."
	@if ! grep -q "source $(PWD)/make_completion.bash" ~/.bashrc; then \
		echo "source $(PWD)/make_completion.bash" >> ~/.bashrc; \
		echo "Added completion to ~/.bashrc. Restart your shell or run 'source ~/.bashrc' to activate."; \
	else \
		echo "Completion already set up in ~/.bashrc."; \
	fi

# =============================================================================
# Help
# =============================================================================

# ── Fleet-standard aliases ────────────────────────────────────────────────────
# Same verbs mean the same thing in every repo; these forward to this
# repo's own targets rather than duplicating their recipes.
.PHONY: fmt lint
fmt: format ## Alias for format
lint: cppcheck ## Alias for cppcheck

# ── Help ──────────────────────────────────────────────────────────────────────
# Targets are self-documenting: text after `## ` on a target line is picked up
# by `make help` automatically, so the listing can never drift from the rules.
.PHONY: help
help: ## Show this help
	@printf '\n  \033[1m%s\033[0m\n' 'Zion-Client'
	@printf '  Usage: make <target> [VAR=value ...]\n'
	@printf '\n'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_.\/-]+:.*?## / { printf "    \033[36m%-24s\033[0m %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@printf '\n'
