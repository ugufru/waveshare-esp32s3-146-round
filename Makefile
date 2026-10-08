SCAD = pocketwatch/pocketwatch.scad

stl: pocketwatch/front.stl pocketwatch/back.stl pocketwatch/plate.stl pocketwatch/plate_pins.stl pocketwatch/back_cup.stl pocketwatch/back_cup_v12.stl

pocketwatch/front.stl: $(SCAD)
	openscad -o $@ -D 'part="front"' $(SCAD)

pocketwatch/back.stl: $(SCAD)
	openscad -o $@ -D 'part="back"' $(SCAD)

pocketwatch/plate.stl: $(SCAD)
	openscad -o $@ -D 'part="plate"' $(SCAD)

pocketwatch/plate_pins.stl: $(SCAD)
	openscad -o $@ -D 'part="plate_pins"' $(SCAD)

pocketwatch/back_cup.stl: $(SCAD)
	openscad -o $@ -D 'part="back_cup"' $(SCAD)

pocketwatch/back_cup_v12.stl: $(SCAD)
	openscad -o $@ -D 'part="back_cup_v12"' $(SCAD)

issues:
	python3 tools/issues_html.py render issues.jsonl issues.html roadmap.jsonl --title="waveshare-esp32s3-146-round issues"

.PHONY: stl issues
