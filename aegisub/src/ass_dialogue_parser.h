
namespace agi { struct Context; }
class AssDialogue;
class Vector2D;

class ParsedAssDialogue {

};

class AssDialogueParser {
	agi::Context *c;

public:
	AssDialogueParser(agi::Context *c);

	/// Get the line's position if it's set, or it's default based on style if not
	Vector2D GetLinePosition(AssDialogue *diag);
	/// Get the line's origin if it's set, or Vector2D::Bad() if not
	Vector2D GetLineOrigin(AssDialogue *diag);
	bool GetLineMove(AssDialogue *diag, Vector2D &p1, Vector2D &p2, int &t1, int &t2);
	void GetLineRotation(AssDialogue *diag, float &rx, float &ry, float &rz);
	void GetLineScale(AssDialogue *diag, Vector2D &scale);
	void GetLineClip(AssDialogue *diag, Vector2D &p1, Vector2D &p2, bool &inverse);
	wxString GetLineVectorClip(AssDialogue *diag, int &scale, bool &inverse);

	int SetOverride(AssDialogue* line, int pos, wxString const& tag, wxString const& value);
};
