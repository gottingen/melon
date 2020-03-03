
.PHONY: test release debug install help clean .clean-build
.DEFAULT_GOAL := help

build_folder = build
current_folder = $(shell pwd)
build_path = $(current_folder)/$(build_folder)

test: ## Run tests
	@echo "start testing ..."
	@cd $(build_path); \
    		make test;

release: .clean-build ## Run tests with race detector
	@echo "buiding release package ..."
	@cd $(build_path); \
		cmake .. -DCMAKE_BUILD_TYPE=Release; \
		make -j2; \
		make package

debug: .clean-build ## Generate test coverage report
	@echo "buiding debug package ..."
	@cd $(build_path); \
    		cmake .. -DCMAKE_BUILD_TYPE=Debug; \
    		make -j2; \
    		make package

install: ## Install abel
	@echo "install abel now ..."
	@cd $(build_path); \
	make install

clean: ## Clean build
	@echo "clean build.."
	@rm -r $(build_path)

.clean-build:
	@echo "clean building env ..."
	@if test -d $(build_path); then \
  		rm -rf $(build_path); \
  		fi
	@mkdir $(build_path)

help:
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'
