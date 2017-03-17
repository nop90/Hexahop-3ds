/*
    Copyright (C) 2005-2007 Tom Beaumont

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


struct HexPuzzle;

class LevelSave
{
	friend struct HexPuzzle;

	// CHECKME: If char is larger than 8 bits (== 1 byte???)
	// the code is no longer big endian save? SWAP16/32 is necessary?
	// Or is a byte always of the same size as char, e.g. 16 bits, so
	// that int16_t is equally saved on big and little endian systems?
	char * bestSolution;
	int32_t bestSolutionLength;
	int32_t bestScore;
	#define NUM_LAST_SCORES 19
	int32_t lastScores[NUM_LAST_SCORES];
	int32_t unlocked;
public:
	LevelSave()
	{
		Clear();
	}
	void Clear()
	{
		unlocked = 0;
		bestSolution = 0;
		bestScore = 0;
		bestSolutionLength = 0;
		memset(lastScores, 0, sizeof(lastScores));	
	}
	void LoadSave(FILE* f, bool save)
	{
		typedef unsigned int _fn(void*, unsigned int, unsigned int, FILE*);
		_fn * fn = save ? (_fn*)fwrite : (_fn*)fread;

		// we write little endian data
		bestSolutionLength = SWAP32(bestSolutionLength);
		bestScore = SWAP32(bestScore);
		for (int i=0; i<NUM_LAST_SCORES; ++i)
			lastScores[i] = SWAP32(lastScores[i]);
		unlocked = SWAP32(unlocked);

		fn(&bestSolutionLength, sizeof(bestSolutionLength), 1, f);
		fn(&bestScore, sizeof(bestScore), 1, f);
		fn(&lastScores, sizeof(lastScores), 1, f);
		fn(&unlocked, sizeof(unlocked), 1, f);		

		bestSolutionLength = SWAP32(bestSolutionLength);
		bestScore = SWAP32(bestScore);
		for (int i=0; i<NUM_LAST_SCORES; ++i)
			lastScores[i] = SWAP32(lastScores[i]);
		unlocked = SWAP32(unlocked);

		if (bestSolutionLength)
		{
			if (!save) SetSolution(bestSolutionLength);
			fn(bestSolution, sizeof(bestSolution[0]), bestSolutionLength, f);
		}
	}

	void Dump()
	{
		for (int j=1; j<NUM_LAST_SCORES; j++)
			if (lastScores[j]==lastScores[0])
				lastScores[j] = 0;

/*		for (int i=0; i<NUM_LAST_SCORES && lastScores[i]; i++)
			if (lastScores[i] != bestScore)
				printf("\t% 8d\n", lastScores[i]);*/
	}
	bool Completed()
	{
		return bestScore != 0;
	}
	bool IsNewCompletionBetter(int score)
	{
		for (int i=0; i<NUM_LAST_SCORES; i++)
		{
			if (lastScores[i]==0)
				lastScores[i] = score;
			if (lastScores[i]==score)
				break;
		}

		if (!Completed())
			return true;

		return score <= bestScore;
	}
	bool BeatsPar(int par)
	{
		if (!Completed())
			return false;
		return bestScore < par;
	}
	bool PassesPar(int par)
	{
		if (!Completed())
			return false;
		return bestScore <= par;
	}
	int GetScore()
	{
		return bestScore;
	}
	void SetScore(int s)
	{
		bestScore = s;
	}
	void SetSolution(int l) { 
		delete [] bestSolution;
		bestSolutionLength = l;
		bestSolution = new char [ l ];
	}
	void SetSolutionStep(int pos, int val)
	{
		bestSolution[pos] = val;
	}
};

class SaveState
{
	struct X : public LevelSave
	{
		X* next;
		char* name;

		X(const char* n, X* nx=0) : next(nx)
		{
			name = new char[strlen(n)+1];
			strcpy(name, n);
		}
		~X()
		{
			delete [] name;
		}
	};

	struct General {
    /// Change big endian data into little endian data, do nothing on little endian systems
		void SwapBytes()
		{
			scoringOn = SWAP32(scoringOn);
			hintFlags = SWAP32(hintFlags);
			completionPercentage = SWAP32(completionPercentage);
			endSequence = SWAP32(endSequence);
			masteredPercentage = SWAP32(masteredPercentage);
			for (unsigned int i=0; i<sizeof(pad)/sizeof(int32_t); ++i)
				pad[i] = SWAP32(pad[i]);
		}
		int32_t scoringOn;
		int32_t hintFlags;
		int32_t completionPercentage;
		int32_t endSequence;
		int32_t masteredPercentage;
		int32_t pad[6];
	};

	X* first;

	void ClearRaw()
	{
		memset(&general, 0, sizeof(general));
		general.hintFlags = 1<<31 | 1<<30;

		X* x=first;
		while (x)
		{
			X* nx = x->next;
			delete x;
			x = nx;
		}
		first = 0;

	}

public:
	
	General general;


	SaveState() : first(0)
	{
		Clear();
	}

	~SaveState()
	{
		ClearRaw();
	}

	void Clear()
	{
		ClearRaw();
		ApplyStuff();
	}

	void GetStuff();
	void ApplyStuff();

	void LoadSave(FILE* f, bool save)
	{
		if (save)
		{
			GetStuff();

			//printf("----\n");

			fputc('2', f);
			general.SwapBytes(); // big==>little endian
			fwrite(&general, sizeof(general), 1, f);
			general.SwapBytes(); // revert changes
			for(X* x=first; x; x=x->next)
			{
				int16_t len = strlen(x->name);
				len = SWAP16(len);
				fwrite(&len, sizeof(len), 1, f);
				len = SWAP16(len);
				fwrite(x->name, 1, len, f);

				x->LoadSave(f,save);

				if (x->Completed())
				{
					//printf("% 8d %s\n", x->GetScore(), x->name);
					x->Dump();
				}
			}
		}
		else
		{
			ClearRaw();
			int v = fgetc(f);
			if (v=='2')
			{
				fread(&general, sizeof(general), 1, f);
				general.SwapBytes();
				v = '1';
			}
			if (v=='1')
			{
				while(!feof(f))
				{
					char temp[1000];
					int16_t len;
					fread(&len, sizeof(len), 1, f);
					len = SWAP16(len);
					if (feof(f)) break;
					fread(temp, len, 1, f);
					temp[len] = 0;
					first = new X(temp, first);

					first->LoadSave(f,save);
				}
			}

			ApplyStuff();
		}
	}

	LevelSave* GetLevel(const char * name, bool create)
	{
		const char * l = strstr(name, "Levels");
		if (l)
			name = l;

		X* x = first;
		if (x && strcmp(name, x->name)==0) return x;
		while (x && x->next)
		{
			if (strcmp(name, x->next->name)==0) return x->next;
			x = x->next;
		}
		if (create)
		{
			X* n = new X(name);
			if (x)
				x->next = n;
			else
				first = n;
			return n;
		}
		else
		{
			return 0;
		}
	}
};
