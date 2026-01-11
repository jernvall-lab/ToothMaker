# Makefile for tribosphenic tooth model
# Builds both C++ and Fortran versions into bin/

SUBDIRS = src/cpp src/fortran

.PHONY: all clean test $(SUBDIRS)

# Build all - continue even if one fails
all:
	@mkdir -p bin
	@failed=""; \
	for dir in $(SUBDIRS); do \
		echo "=== Building $$dir ==="; \
		if $(MAKE) -C $$dir; then \
			echo "=== $$dir: OK ==="; \
		else \
			echo "=== $$dir: FAILED ==="; \
			failed="$$failed $$dir"; \
		fi; \
		echo ""; \
	done; \
	if [ -n "$$failed" ]; then \
		echo "Failed builds:$$failed"; \
		exit 1; \
	fi

# Clean all subdirectories
clean:
	@for dir in $(SUBDIRS); do \
		echo "=== Cleaning $$dir ==="; \
		$(MAKE) -C $$dir clean; \
	done

# Run tests (C++ only for now)
test:
	@$(MAKE) -C src/cpp test

# Individual targets
cpp:
	@mkdir -p bin
	$(MAKE) -C src/cpp

fortran:
	@mkdir -p bin
	$(MAKE) -C src/fortran
