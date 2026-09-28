SCAD = pocketwatch/pocketwatch.scad

stl: pocketwatch/front.stl pocketwatch/back.stl

pocketwatch/front.stl: $(SCAD)
	openscad -o $@ -D 'part="front"' $(SCAD)

pocketwatch/back.stl: $(SCAD)
	openscad -o $@ -D 'part="back"' $(SCAD)

issues:
	python3 tools/issues_html.py render issues.jsonl issues.html roadmap.jsonl --title="waveshare-esp32s3-146-round issues"

.PHONY: stl issues
