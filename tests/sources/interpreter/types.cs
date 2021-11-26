function ScriptObject::test(%this) {
	echo(%this.hey);
}

%first = new ScriptObject() {
	hey = "how are you doing";
};

%second = new ScriptObject() {
	hey = "not very well";
};

%third = new ScriptObject() {
	hey = "superb";
};

%obj = new Array();
for(%i = 0; %i < 4; %i++) {
	%obj.push(null);
}

%objNumber = new Array();
for(%i = 0; %i < 4; %i++) {
	%objNumber.push(null);
}

%objString = new Array();
for(%i = 0; %i < 4; %i++) {
	%objString.push(null);
}

%obj[0] = %first;
%obj[1] = %second;
%obj[2] = %third;
%obj[3] = -1;

%objNumber[0] = 0;
%objNumber[1] = 1;
%objNumber[2] = 2;
%objNumber[3] = 3;

%objString[0] = "0";
%objString[1] = "1";
%objString[2] = "2";
%objString[3] = "3";

for(%i = 0; %i < 4; %i++) {
	echo(%obj[%i].hey);
	echo(%objNumber[%i].hey);
	echo(%objString[%i].hey);

	%obj[%i].test();
	%objNumber[%i].test();
	%objString[%i].test();

	echo(-%obj[%i]);
	echo(-%objNumber[%i]);
	echo(-%objString[%i]);
}