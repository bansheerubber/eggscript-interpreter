%test = new Array();
%test.push(10, 193, 4981, 85, 19, 2891, 985, 91, 76, 78);

echo("order:");
for(%i = 0; %i < %test.size(); %i++) {
	echo(%i @ ":" SPC %test[%i]);
}

%test.insert(4, "oh hey");
echo("new order:");
for(%i = 0; %i < %test.size(); %i++) {
	echo(%i @ ":" SPC %test[%i]);
}

%test.remove(4);
echo("new new order:");
for(%i = 0; %i < %test.size(); %i++) {
	echo(%i @ ":" SPC %test[%i]);
}

echo("indices:");
for(%i = 0; %i < %test.size(); %i++) {
	echo(%i @ ":" SPC %test.index(%test[%i]));
}
