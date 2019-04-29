BIN_DIR := bin
SRC_DIR := src

AS = nasm

BINS := $(BIN_DIR)/xlink
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(filter-out \
 $(patsubst $(BIN_DIR)/%,$(SRC_DIR)/%.c,$(BINS)),$(wildcard $(SRC_DIR)/*.c)))
MODS := $(patsubst $(SRC_DIR)/%.asm,$(BIN_DIR)/%.o,$(wildcard $(SRC_DIR)/*.asm))

CFLAGS := -O2 -Wno-parenthesis -Wno-overlength-strings

ASFLAGS := -f obj -i src/

LIBS := -lm

all: $(BINS) $(OBJS) $(MODS)

guard=@mkdir -p $(@D)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(guard)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/%.o: $(SRC_DIR)/%.asm
	$(guard)
	$(AS) $(ASFLAGS) -o $@ $<

$(SRC_DIR)/stubs.h: $(MODS)
	@echo '/* Generated file, do not commit */' > $@
	@echo 'const xlink_file XLINK_STUB_MODULES[] = {' >> $@
	@$(foreach s,$(patsubst $(BIN_DIR)/%.o,%,$^), \
		$(eval SIZE = $(shell stat -c %s $(BIN_DIR)/$s.o)) \
		echo '  { ' >> $@; \
		echo '    "$s.o",' >> $@; \
		echo '    $(SIZE), 0,' >> $@; \
		echo '    (unsigned char[]) {' >> $@; \
		xxd -i - < $(BIN_DIR)/$s.o | sed -e s'/^/    /' >> $@; \
		echo '    }' >> $@; \
		echo '  },' >> $@; \
	)
	@echo '};' >> $@; \

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(SRC_DIR)/stubs.h $(OBJS)
	$(guard)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

clean:
	rm -rf $(BIN_DIR) $(SRC_DIR)/stubs.h
