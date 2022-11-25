%!
% Copyright (c) 1989 SecureWare, Inc.  All Rights Reserved.
%
% @(#)SWbanner.pro	10.1 15:21:52 2/16/90 SecureWare
%
% pagewidth pageheight landscape [ (ProtectClass) ] [ (ProtectAs) ]
% [ (InfoLabel) ] [ (BannerLabel) ] [ (HandleVia)] (PageLabel) LabelSetup
%
% (user@host) (username) (jobid) (title) (printer) (date) Banner

/LabelSetup {

	/$LabelDict$ 60 dict def

	$LabelDict$ begin

		/PageLabel exch def
		/HandleVia exch def
		/BannerLabel exch def
		/InfoLabel exch def
		/ProtectAs exch def
		/ProtectClass exch def
		/Orientation exch def
		/PageHeight exch def
		/PageWidth exch def

		/HMargin 19 def
		/VMargin 8 def
		/BFontBold /Palatino-Bold findfont 12 scalefont def
		/BFontReg /Courier findfont 12 scalefont def

		/FontScale { % font-key normal-scale array FontScale -
			length idiv exch findfont exch scalefont
		} def

		/DataLineCount  {
			HandleVia length
			BannerLabel length
			InfoLabel length
			ProtectAs length
			add add add
		} def

		/BFontBoldScale {
			/Courier-Bold 12 3 -1 roll FontScale
		} def

		/BFontRegScale {
			/Courier 12 3 -1 roll FontScale
		} def

		PageLabel 0 get length 0 ne {
			Orientation 0 ne
				{	/VMarg HMargin def /HMarg VMargin def
					/PgHeight PageWidth def /PgWidth PageHeight def
					Orientation 0 gt
						{ 0 PgHeight neg matrix translate 90 matrix rotate }
						{ PgWidth neg 0 matrix translate -90 matrix rotate }
					ifelse
					matrix concatmatrix
					matrix defaultmatrix
					matrix concatmatrix
				}
				{	/VMarg VMargin def /HMarg HMargin def
					/PgHeight PageHeight def /PgWidth PageWidth def
					matrix defaultmatrix
				}
			ifelse
			/LabelMatrix exch def

			/ArrayStringWidth { % array ArrayStringWidth width
				/ASWArray exch def
				/CurMaxWidth 0 def
				0 1 ASWArray length 1 sub {
					ASWArray exch get stringwidth pop
					dup CurMaxWidth gt
						{ /CurMaxWidth exch def }
						{ pop }
					ifelse
				} for
				CurMaxWidth
			} def

			/LabelBarGray .05 def
			/LabelGray 1 def
			/LabelFont /Courier-Bold findfont dup
				gsave setfont PgWidth HMarg 2 mul sub
					PageLabel 0 get stringwidth pop div grestore
				12 gt PageLabel length 1 le and { 12 } { 6 } ifelse
				dup /LabelPS exch def
				scalefont def
			/LabelBarWidth gsave LabelFont setfont PageLabel ArrayStringWidth
				9 add grestore def
			/LabelBarHeight LabelPS dup 12 lt { 0 } { 2 } ifelse add def

			/ShowLabel {	% string Vpos ShowLabel -
				gsave
					exch dup /ThisString exch def exch
					dup
					initgraphics
					LabelMatrix setmatrix
					LabelBarGray setgray
					PgWidth LabelBarWidth sub 2 div
						dup HMarg lt { pop HMarg } if exch moveto
					LabelBarWidth 0 rlineto
					0 LabelBarHeight neg rlineto
					LabelBarWidth neg 0 rlineto
					closepath fill
					LabelFont setfont
					ThisString stringwidth pop PgWidth exch sub 2 div
						dup HMarg lt { pop HMarg } if
						exch LabelBarHeight sub
							LabelPS 3 div add moveto
					LabelGray setgray
					show
				grestore
			} def

			/DisplayLabelTop {	% Vpos DisplayLabelTop -
				/PassedVpos exch def
				0 1 PageLabel length 1 sub {
					dup PageLabel exch get exch
					Orientation 0 eq
						{ 0 }
						{ 3.9 LabelPS div }
					ifelse sub
					LabelBarHeight mul PassedVpos exch sub
					ShowLabel
				} for
			} def

			/DisplayLabelBot {	% Vpos DisplayLabelBot -
				/PassedVpos exch def
				0 1 PageLabel length 1 sub {
					dup PageLabel exch get exch
					PageLabel length exch sub
					Orientation 0 eq
						{ 0 }
						{ 3.9 LabelPS div }
					ifelse sub
					LabelBarHeight mul PassedVpos exch add
					ShowLabel
				} for
			} def
			userdict begin
				/showpage {
					$LabelDict$ begin
						PgHeight VMarg sub DisplayLabelTop
						VMarg DisplayLabelBot
					end
					systemdict begin showpage end
				} def

				/copypage {
					$LabelDict$ begin
						PgHeight VMarg sub DisplayLabelTop
						VMarg DisplayLabelBot
					end
					systemdict begin copypage end
				} def
			end
		} if

		/Bar {			% Vpos Bar -
			gsave
				.5 setgray
				newpath
				0 exch moveto
				PageWidth 0 rlineto
				0 -18 rlineto
				PageWidth neg 0 rlineto
				closepath fill
			grestore
		} def

		/CenterShow {	% string Vpos CenterShow -
			exch dup PageWidth exch stringwidth pop sub 2 div
			3 -1 roll moveto show
		} def

		/ArrayShow {	% array Vpos CenterShowArray -
			/SaveVpos Vpos def
			/Vpos exch def
			{ Vpos CenterShow NewLine } forall
			/Vpos SaveVpos def
		} def

		/ArrayShowHere {	% array CenterShowArray -
			{ Vpos CenterShow NewLine } forall
		} def

		/NewLine { /Vpos Vpos Leading sub def 72 Vpos moveto } def

		/LabelBanner {

			ProtectClass BFontBold setfont
			ProtectClass PageHeight VMargin 12 add sub ArrayShow
			ProtectClass VMargin ProtectClass length 1 sub Leading
					mul add ArrayShow

			BFontReg setfont
			(The sensitivity label of the user is:) Vpos CenterShow NewLine
			NewLine

			ProtectAs BFontBold setfont
			ProtectAs ArrayShowHere BFontBold setfont NewLine
			NewLine

			BFontReg setfont
			(Unless manually reviewed and downgraded) Vpos CenterShow NewLine
			NewLine
			NewLine
			(The system has labeled this data:) Vpos CenterShow NewLine
			NewLine

			InfoLabel BFontBold setfont
			InfoLabel ArrayShowHere BFontBold setfont NewLine
			NewLine
			NewLine
			BannerLabel BFontBold setfont
			BannerLabel ArrayShowHere BFontBold setfont NewLine
			NewLine
			HandleVia BFontBold setfont
			HandleVia ArrayShowHere

			% The next 4 lines were added by Cupertino Unix Lab to the SecureWare's
                        % version to print the file SL at the Bottom of the page, see the man page.

			PageLabel BFontBold setfont
			PageLabel 0 VMargin 12 add add ArrayShow
			PageLabel VMargin PageLabel length 1 sub Leading
					mul add ArrayShow
		} def

		/Banner {
			/date exch def
			/printer exch def
			/title exch def
			/jobid exch def
			/User exch def
			/user exch def

			statusdict /printername known
				{ /pn 31 string statusdict /printername get exec def }
				{ /pn (PostScript) def }
			ifelse

			/ShowLine { 72 Vpos moveto show NewLine } def
			/ShowStr { show (       ) show } def

			/Leading 30 def
			/Vpos PageHeight 54 sub def

			Vpos Bar
			/Vpos Vpos 32 sub def

			/Helvetica-Bold findfont 18 scalefont setfont

			NewLine
			user ShowStr User ShowStr NewLine
			title ShowLine

			/Helvetica findfont 14 scalefont setfont
			date ShowLine
			(Job) ShowStr jobid ShowStr
			(printed on ) show printer show
			( \() show pn show (\)) show NewLine

			Vpos Bar
			72 Bar

			/Leading 14 def
			/Vpos Vpos dup 72 sub 178 Leading 1 sub DataLineCount mul add
				sub 2 div sub def

			LabelBanner
 
			systemdict begin showpage end

		} def

		/Trailer {
			/Vpos PageHeight 54 sub def
			/Leading 14 def

			Vpos Bar
			72 Bar

			/Vpos Vpos 216 sub def

			LabelBanner

			systemdict begin showpage end
		} def

	end

} def
