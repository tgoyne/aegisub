
namespace agi { struct Context; }
class AssDialogue;
class Vector2D;


enum ASS_BlockType {
	BLOCK_BASE,
	BLOCK_PLAIN,
	BLOCK_OVERRIDE,
	BLOCK_DRAWING
};

class AssOverrideParameter;
class AssOverrideTag;

/// DOCME
/// @class AssDialogueBlock

/// @brief AssDialogue Blocks
///
/// A block is each group in the text field of an AssDialogue
/// @verbatim
///  Yes, I {\i1}am{\i0} here.
///
/// Gets split in five blocks:
///  "Yes, I " (Plain)
///  "\\i1"     (Override)
///  "am"      (Plain)
///  "\\i0"     (Override)
///  " here."  (Plain)
///
/// Also note how {}s are discarded.
/// Override blocks are further divided in AssOverrideTags.
///
/// The GetText() method generates a new value for the "text" field from
/// the other fields in the specific class, and returns the new value.
/// @endverbatim
class AssDialogueBlock {
protected:
	wxString text;
public:
	AssDialogueBlock(wxString const& text) : text(text) { }
	virtual ~AssDialogueBlock() { }

	virtual ASS_BlockType GetType() = 0;
	virtual wxString GetText() { return text; }
};

class AssDialogueBlockPlain : public AssDialogueBlock {
public:
	ASS_BlockType GetType() { return BLOCK_PLAIN; }
	AssDialogueBlockPlain(wxString const& text = "") : AssDialogueBlock(text) { }
};

class AssDialogueBlockDrawing : public AssDialogueBlock {
public:
	int Scale;

	ASS_BlockType GetType() { return BLOCK_DRAWING; }
	AssDialogueBlockDrawing(wxString const& text = "") : AssDialogueBlock(text) { }
	void TransformCoords(int trans_x,int trans_y,double mult_x,double mult_y);
};

class AssDialogueBlockOverride : public AssDialogueBlock {
public:
	AssDialogueBlockOverride(wxString const& text = "") : AssDialogueBlock(text) { }
	~AssDialogueBlockOverride();

	std::vector<AssOverrideTag*> Tags;


	ASS_BlockType GetType() { return BLOCK_OVERRIDE; }
	wxString GetText();
	void ParseTags();
	void AddTag(wxString const& tag);

	/// Type of callback function passed to ProcessParameters
	typedef void (*ProcessParametersCallback)(wxString,int,AssOverrideParameter*,void *);

	/// @brief Process parameters via callback 
	/// @param callback The callback function to call per tag parameter
	/// @param userData User data to pass to callback function
	void ProcessParameters(ProcessParametersCallback callback, void *userData);
};

class ParsedAssDialogue : private std::vector<AssDialogueBlock*> {
	AssDialogue *line;

	ParsedAssDialogue(ParsedAssDialogue const&);
	ParsedAssDialogue& operator=(ParsedAssDialogue const&);
public:
	explicit ParsedAssDialogue(AssDialogue *line);
	~ParsedAssDialogue();

	using std::vector<AssDialogueBlock*>::size;
	using std::vector<AssDialogueBlock*>::operator[];
	using std::vector<AssDialogueBlock*>::begin;
	using std::vector<AssDialogueBlock*>::end;
	using std::vector<AssDialogueBlock*>::clear;
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
