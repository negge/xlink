BIN_DIR := bin
SRC_DIR := src

BINS := $(BIN_DIR)/xlink
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(filter-out \
 $(patsubst $(BIN_DIR)/%,$(SRC_DIR)/%.c,$(BINS)),$(wildcard $(SRC_DIR)/*.c)))

CFLAGS := -O2 -Wno-parenthesis -Wno-overlength-strings

all: $(BINS) $(OBJS)

guard=@mkdir -p $(@D)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(guard)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/%: $(SRC_DIR)/%.c $(OBJS)
	$(guard)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@ $(LIBS)

clean:
	rm -rf $(BIN_DIR)
