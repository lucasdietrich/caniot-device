.PHONY: format

format:
	find src -iname *.h -o -iname *.c -o -iname *.cpp | xargs clang-format -i