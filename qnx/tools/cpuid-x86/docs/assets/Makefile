# Renders every PlantUML source in this directory to SVG.
# Each X.puml goes to directory X/, with one X/<name>.svg per @startuml <name> block.
# Local:  requires the plantuml command (Ubuntu: sudo apt install plantuml).
# Remote: make PLANTUML_SERVER=https://www.plantuml.com/plantuml  (uses curl, no local plantuml).
# Grouped targets (&:) need GNU make 4.3 or later.

PLANTUML        ?= plantuml
PLANTUML_SERVER ?=
PUML            := $(wildcard *.puml)

diagram_names = $(shell awk '$$1 == "@startuml" && NF > 1 { print $$2 }' $(1))
SVG           := $(foreach p,$(PUML),$(addprefix $(basename $(p))/,$(addsuffix .svg,$(call diagram_names,$(p)))))

.PHONY: all clean

all: $(SVG)

define PUML_RULE
$(addprefix $(basename $(1))/,$(addsuffix .svg,$(call diagram_names,$(1)))) &: $(1)
	@mkdir -p $(basename $(1))
ifeq ($(PLANTUML_SERVER),)
	$(PLANTUML) -tsvg -o $(basename $(1)) $$<
else
	$(foreach n,$(call diagram_names,$(1)),awk -v n=$(n) '$$$$1 == "@startuml" { p = ($$$$2 == n) } p { print } $$$$1 == "@enduml" { p = 0 }' $$< | curl -fsS -X POST --data-binary @- $(PLANTUML_SERVER)/svg/ -o $(basename $(1))/$(n).svg || exit 1;)
endif
endef

$(foreach p,$(PUML),$(eval $(call PUML_RULE,$(p))))

clean:
	rm -f $(SVG)
