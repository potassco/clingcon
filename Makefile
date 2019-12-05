all:
	@cd build/debug; ninja

%:
	@cd build/debug; ninja $@
