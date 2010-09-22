sub HelloPL {
	my ($main_conf,$real_inputs,$real_outputs) = @_;
	
	$real_outputs->{"Result"}->{"value"}=$real_inputs->{"a"}->{"value"};
	return 3;
}

