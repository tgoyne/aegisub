#include "config.h"

#include "ass_dialogue_parser.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "include/aegisub/context.h"
#include "vector2d.h"

typedef const std::vector<AssOverrideParameter*> * param_vec;

// Parse line on creation and unparse at the end of scope
struct scoped_tag_parse {
	AssDialogue *diag;
	scoped_tag_parse(AssDialogue *diag) : diag(diag) { diag->ParseASSTags(); }
	~scoped_tag_parse() { diag->ClearBlocks(); }
};

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

	scoped_tag_parse parse(diag);

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
	scoped_tag_parse parse(diag);
	return vec_or_bad(find_tag(diag, "\\org"), 0, 1);
}

bool AssDialogueParser::GetLineMove(AssDialogue *diag, Vector2D &p1, Vector2D &p2, int &t1, int &t2) {
	scoped_tag_parse parse(diag);

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

	scoped_tag_parse parse(diag);

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

	scoped_tag_parse parse(diag);

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

	scoped_tag_parse parse(diag);
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
	scoped_tag_parse parse(diag);

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

void AssDialogueParser::SetOverride(AssDialogue* line, int pos, wxString const& tag, wxString const& value) {
	if (!line) return;

	wxString removeTag;
	if (tag == "\\1c") removeTag = "\\c";
	else if (tag == "\\frz") removeTag = "\\fr";
	else if (tag == "\\pos") removeTag = "\\move";
	else if (tag == "\\move") removeTag = "\\pos";
	else if (tag == "\\clip") removeTag = "\\iclip";
	else if (tag == "\\iclip") removeTag = "\\clip";

	wxString insert = tag + value;

	// Get block at start
	line->ParseASSTags();
	AssDialogueBlock *block = line->Blocks.front();

	// Get current block as plain or override
	assert(dynamic_cast<AssDialogueBlockDrawing*>(block) == NULL);

	if (dynamic_cast<AssDialogueBlockPlain*>(block))
		line->Text = "{" + insert + "}" + line->Text;
	else if (AssDialogueBlockOverride *ovr = dynamic_cast<AssDialogueBlockOverride*>(block)) {
		// Remove old of same
		for (size_t i = 0; i < ovr->Tags.size(); i++) {
			wxString name = ovr->Tags[i]->Name;
			if (tag == name || removeTag == name) {
				delete ovr->Tags[i];
				ovr->Tags.erase(ovr->Tags.begin() + i);
				i--;
			}
		}
		ovr->AddTag(insert);

		line->UpdateText();
	}
}

