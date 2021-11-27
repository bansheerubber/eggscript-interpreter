function ScriptObject::test1(%this) {
	echo("ran test1");
	return new ScriptObject();
}

function ScriptObject::test2(%this) {
	echo("ran test2");
	return new ScriptObject();
}

function ScriptObject::test3(%this) {
	echo("ran test3");
	return 5;
}

%test = new ScriptObject();

%test.test1().test2().test3();
echo(%test.test1().test2().test3());
