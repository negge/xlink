BIN_DIR := bin
SRC_DIR := src

AS = nasm

BINS := $(BIN_DIR)/xlink
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(filter-out \
 $(patsubst $(BIN_DIR)/%,$(SRC_DIR)/%.c,$(BINS)),$(wildcard $(SRC_DIR)/*.c)))
STBS := $(patsubst $(SRC_DIR)/%.asm,$(BIN_DIR)/%.o,$(wildcard $(SRC_DIR)/*.asm))

CFLAGS := -O2 -Wno-parenthesis -Wno-overlength-strings

ASFLAGS := -f obj -i src/

LIBS := -lm

all: $(BINS) $(OBJS) $(STBS)

guard=@mkdir -p $(@D)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(guard)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o: $(SRC_DIR)/%.asm
	$(guard)
	$(AS) $(ASFLAGS) -o $@ $<

$(SRC_DIR)/stubs.h: $(STBS)
	@echo '/* Generated file, do not commit */' > $@
	@$(foreach s,$(patsubst $(BIN_DIR)/%.o,%,$^), \
		$(eval SIZE = $(shell stat -c %s $(BIN_DIR)/$s.o)) \
		$(eval NAME = $(shell echo $s | tr '[.a-z]' '[_A-Z]')) \
		echo 'const xlink_file $(NAME)_MODULE = {' >> $@; \
		echo '  "$s.o",' >> $@; \
		echo '  $(SIZE), 0,' >> $@; \
		echo '  (char[]){' >> $@; \
		xxd -i - < $(BIN_DIR)/$s.o >> $@; \
		echo '  }' >> $@; \
		echo '};' >> $@; \
	)

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/stubs.h
	$(guard)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

clean:
	rm -rf $(BIN_DIR) $(SRC_DIR)/stubs.h
