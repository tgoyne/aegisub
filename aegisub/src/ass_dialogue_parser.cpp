#include "config.h"

#include "ass_dialogue_parser.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "include/aegisub/context.h"
#include "utils.h"
#include "vector2d.h"

ParsedAssDialogue::ParsedAssDialogue(AssDialogue *line)
: line(line)
{
	// Empty line, make an empty block
	if (line->Text.empty()) {
		push_back(new AssDialogueBlockPlain);
		return;
	}

	int drawingLevel = 0;

	wxString::iterator end = line->Text.end();
	for (wxString::iterator it = line->Text.begin(); it != end; ) {
		// Overrides block
		if (*it == '{') {
			++it;

			// Get contents of block
			wxString::iterator block_end = std::find(it, end, '}');
			
			if (std::distance(it, block_end) <= 1)
				push_back(new AssDialogueBlockOverride);
			//We've found an override block with no backslashes
			//We're going to assume it's a comment and not consider it an override block
			//Currently we'll treat this as a plain text block, but feel free to create a new class
			else if (std::find(it, block_end, '\\') == block_end)
				push_back(new AssDialogueBlockPlain("{" + wxString(it, block_end) + "}"));
			else {
				AssDialogueBlockOverride *block = new AssDialogueBlockOverride(wxString(it, block_end));
				block->ParseTags();
				push_back(block);

				// Look for \p in block
				for (std::vector<AssOverrideTag*>::iterator  curTag = block->Tags.begin(); curTag != block->Tags.end(); ++curTag) {
					if ((*curTag)->Name == "\\p")
						drawingLevel = (*curTag)->Params[0]->Get<int>(0);
				}
			}

			it = block_end;
			if (it != end) ++it;
		}
		// Plain-text/drawing block
		else {
			wxString::iterator next = std::find(it, end, '{');

			// Plain-text
			if (drawingLevel == 0) {
				push_back(new AssDialogueBlockPlain(wxString(it, next)));
			}
			// Drawing
			else {
				AssDialogueBlockDrawing *block = new AssDialogueBlockDrawing(wxString(it, next));
				block->Scale = drawingLevel;
				push_back(block);
			}

			it = next;
		}
	}
}

ParsedAssDialogue::~ParsedAssDialogue() {
	delete_clear(*this);
}

typedef const std::vector<AssOverrideParameter*> * param_vec;

// Find a tag's parameters in a line or return NULL if it's not found
static param_vec find_tag(const AssDialogue *line, wxString tag_name) {
	for (size_t i = 0; i < line->Blocks.size(); ++i) {
		const AssDialogueBlockOverride *ovr = dynamic_cast<const AssDialogueBlockOverride*>(line->Blocks[i]);
		if (!ovr) continue;

		for (size_t j = 0; j < ovr->Tags.size(); ++j) {
			if (ovr->Tags[j]->Name == tag_name)
				return &ovr->Tags[j]->Params;
		}
	}

	return 0;
}

// Get a Vector2D from the given tag parameters, or Vector2D::Bad() if they are not valid
static Vector2D vec_or_bad(param_vec tag, size_t x_idx, size_t y_idx) {
	if (!tag ||
		tag->size() <= x_idx || tag->size() <= y_idx ||
		(*tag)[x_idx]->omitted || (*tag)[y_idx]->omitted ||
		(*tag)[x_idx]->GetType() == VARDATA_NONE || (*tag)[y_idx]->GetType() == VARDATA_NONE)
	{
		return Vector2D();
	}
	return Vector2D((*tag)[x_idx]->Get<float>(), (*tag)[y_idx]->Get<float>());
}

AssDialogueParser::AssDialogueParser(agi::Context *c)
: c(c)
{
}

Vector2D AssDialogueParser::GetLinePosition(AssDialogue *diag) {
	int script_w, script_h;
	c->ass->GetResolution(script_w, script_h);
	Vector2D script_res(script_w, script_h);

	ParsedAssDialogue parse(diag);

	if (Vector2D ret = vec_or_bad(find_tag(diag, "\\pos"), 0, 1)) return ret;
	if (Vector2D ret = vec_or_bad(find_tag(diag, "\\move"), 0, 1)) return ret;

	// Get default position
	int margin[4];
	std::copy(diag->Margin, diag->Margin + 4, margin);
	int align = 2;

	if (AssStyle *style = c->ass->GetStyle(diag->Style)) {
		align = style->alignment;
		for (int i = 0; i < 4; i++) {
			if (margin[i] == 0)
				margin[i] = style->Margin[i];
		}
	}

	param_vec align_tag;
	if ((align_tag = find_tag(diag, "\\an")) && !(*align_tag)[0]->omitted)
		align = (*align_tag)[0]->Get<int>();
	else if ((align_tag = find_tag(diag, "\\a"))) {
		align = (*align_tag)[0]->Get<int>(2);

		// \a -> \an values mapping
		static int align_mapping[] = { 2, 1, 2, 3, 7, 7, 8, 9, 7, 4, 5, 6 };
		if (static_cast<size_t>(align) < sizeof(align_mapping) / sizeof(int))
			align = align_mapping[align];
		else
			align = 2;
	}

	// Alignment type
	int hor = (align - 1) % 3;
	int vert = (align - 1) / 3;

	// Calculate positions
	int x, y;
	if (hor == 0)
		x = margin[0];
	else if (hor == 1)
		x = (script_res.X() + margin[0] - margin[1]) / 2;
	else
		x = margin[1];

	if (vert == 0)
		y = script_res.Y() - margin[2];
	else if (vert == 1)
		y = script_res.Y() / 2;
	else
		y = margin[2];

	return Vector2D(x, y);
}

Vector2D AssDialogueParser::GetLineOrigin(AssDialogue *diag) {
	ParsedAssDialogue parse(diag);
	return vec_or_bad(find_tag(diag, "\\org"), 0, 1);
}

bool AssDialogueParser::GetLineMove(AssDialogue *diag, Vector2D &p1, Vector2D &p2, int &t1, int &t2) {
	ParsedAssDialogue parse(diag);

	param_vec tag = find_tag(diag, "\\move");
	if (!tag)
		return false;

	p1 = vec_or_bad(tag, 0, 1);
	p2 = vec_or_bad(tag, 2, 3);
	// VSFilter actually defaults to -1, but it uses <= 0 to check for default and 0 seems less bug-prone
	t1 = (*tag)[4]->Get<int>(0);
	t2 = (*tag)[5]->Get<int>(0);

	return p1 && p2;
}

void AssDialogueParser::GetLineRotation(AssDialogue *diag, float &rx, float &ry, float &rz) {
	rx = ry = rz = 0.f;

	if (AssStyle *style = c->ass->GetStyle(diag->Style))
		rz = style->angle;

	ParsedAssDialogue parse(diag);

	if (param_vec tag = find_tag(diag, "\\frx"))
		rx = tag->front()->Get<float>(rx);
	if (param_vec tag = find_tag(diag, "\\fry"))
		ry = tag->front()->Get<float>(ry);
	if (param_vec tag = find_tag(diag, "\\frz"))
		rz = tag->front()->Get<float>(rz);
	else if ((tag = find_tag(diag, "\\fr")))
		rz = tag->front()->Get<float>(rz);
}

void AssDialogueParser::GetLineScale(AssDialogue *diag, Vector2D &scale) {
	float x = 100.f, y = 100.f;

	if (AssStyle *style = c->ass->GetStyle(diag->Style)) {
		x = style->scalex;
		y = style->scaley;
	}

	ParsedAssDialogue parse(diag);

	if (param_vec tag = find_tag(diag, "\\fscx"))
		x = tag->front()->Get<float>(x);
	if (param_vec tag = find_tag(diag, "\\fscy"))
		y = tag->front()->Get<float>(y);

	scale = Vector2D(x, y);
}

void AssDialogueParser::GetLineClip(AssDialogue *diag, Vector2D &p1, Vector2D &p2, bool &inverse) {
	int script_w, script_h;
	c->ass->GetResolution(script_w, script_h);
	Vector2D script_res(script_w, script_h);
	inverse = false;

	ParsedAssDialogue parse(diag);
	param_vec tag = find_tag(diag, "\\iclip");
	if (tag)
		inverse = true;
	else
		tag = find_tag(diag, "\\clip");

	if (tag && tag->size() == 4) {
		p1 = vec_or_bad(tag, 0, 1);
		p2 = vec_or_bad(tag, 2, 3);
	}
	else {
		p1 = Vector2D(0, 0);
		p2 = script_res - 1;
	}
}

wxString AssDialogueParser::GetLineVectorClip(AssDialogue *diag, int &scale, bool &inverse) {
	ParsedAssDialogue parse(diag);

	scale = 1;
	inverse = false;

	param_vec tag = find_tag(diag, "\\iclip");
	if (tag)
		inverse = true;
	else
		tag = find_tag(diag, "\\clip");

	if (tag && tag->size() == 4) {
		return wxString::Format("m %d %d l %d %d %d %d %d %d",
			(*tag)[0]->Get<int>(), (*tag)[1]->Get<int>(),
			(*tag)[2]->Get<int>(), (*tag)[1]->Get<int>(),
			(*tag)[2]->Get<int>(), (*tag)[3]->Get<int>(),
			(*tag)[0]->Get<int>(), (*tag)[3]->Get<int>());
	}
	if (tag) {
		scale = (*tag)[0]->Get<int>(scale);
		return (*tag)[1]->Get<wxString>("");
	}

	return "";
}

/// Get the block index in the text of the position
int block_at_pos(wxString const& text, int pos) {
	int n = 0;
	int max = text.size() - 1;
	for (int i = 0; i <= pos && i <= max; ++i) {
		if (i > 0 && text[i] == '{')
			n++;
		if (text[i] == '}' && i != max && i != pos && i != pos -1 && (i+1 == max || text[i+1] != '{'))
			n++;
	}

	return n;
}

int AssDialogueParser::SetOverride(AssDialogue* line, int pos, wxString const& tag, wxString const& value) {
	if (!line) return 0;

	ParsedAssDialogue parse(line);
	int blockn = block_at_pos(line->Text, pos);

	AssDialogueBlockPlain *plain = 0;
	AssDialogueBlockOverride *ovr = 0;
	while (blockn >= 0) {
		AssDialogueBlock *block = line->Blocks[blockn];
		if (dynamic_cast<AssDialogueBlockDrawing*>(block))
			--blockn;
		else if ((plain = dynamic_cast<AssDialogueBlockPlain*>(block))) {
			// Cursor is in a comment block, so try the previous block instead
			if (plain->GetText().StartsWith("{")) {
				--blockn;
				pos = line->Text.rfind('{', pos);
			}
			else
				break;
		}
		else {
			ovr = dynamic_cast<AssDialogueBlockOverride*>(block);
			assert(ovr);
			break;
		}
	}

	// If we didn't hit a suitable block for inserting the override just put
	// it at the beginning of the line
	if (blockn < 0)
		pos = 0;

	wxString insert = tag + value;
	if (plain || blockn < 0) {
		line->Text = line->Text.Left(pos) + "{" + insert + "}" + line->Text.Mid(pos);
		return insert.size() + 2;
	}
	else if(ovr) {
		wxString alt;
		if (tag == "\\1c") alt = "\\c";
		else if (tag == "\\frz") alt = "\\fr";
		else if (tag == "\\pos") alt = "\\move";
		else if (tag == "\\move") alt = "\\pos";
		else if (tag == "\\clip") alt = "\\iclip";
		else if (tag == "\\iclip") alt = "\\clip";

		int shift = insert.size();
		bool found = false;
		for (size_t i = 0; i < ovr->Tags.size(); i++) {
			wxString name = ovr->Tags[i]->Name;
			if (tag == name || alt == name) {
				shift -= ((wxString)*ovr->Tags[i]).size();
				delete ovr->Tags[i];
				if (found) {
					ovr->Tags.erase(ovr->Tags.begin() + i);
					i--;
				}
				else {
					ovr->Tags[i] = new AssOverrideTag(insert);
					found = true;
				}
			}
		}
		if (!found)
			ovr->AddTag(insert);

		line->UpdateText();
		return shift;
	}
	else {
		assert(false);
		return 0;
	}
}
