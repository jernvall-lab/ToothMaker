# Makefile for tribosphenic tooth model
# Builds both C++ and Fortran versions

SUBDIRS = cpp fortran

.PHONY: all clean test $(SUBDIRS)

# Build all - continue even if one fails
all:
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
	@$(MAKE) -C cpp test

# Individual targets
cpp:
	$(MAKE) -C cpp

fortran:
	$(MAKE) -C fortran
