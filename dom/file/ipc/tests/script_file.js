Cu.importGlobalProperties(["File"]);

addMessageListener("file.open", function (e) {
  var testFile = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIDirectoryService)
                   .QueryInterface(Ci.nsIProperties)
                   .get("ProfD", Ci.nsIFile);
  testFile.append("ipc_fileReader_testing");
  testFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0o600);

  var outStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                      .createInstance(Components.interfaces.nsIFileOutputStream);
  outStream.init(testFile, 0x02 | 0x08 | 0x20, // write, create, truncate
                 0666, 0);

  var fileData = "Hello World!";
  outStream.write(fileData, fileData.length);
  outStream.close();

  File.createFromNsIFile(testFile).then(function(file) {
    sendAsyncMessage("file.opened", { file });
  });
});

addMessageListener("emptyfile.open", function (e) {
  var testFile = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIDirectoryService)
                   .QueryInterface(Ci.nsIProperties)
                   .get("ProfD", Ci.nsIFile);
  testFile.append("ipc_fileReader_testing");
  testFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0o600);

  var outStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                      .createInstance(Components.interfaces.nsIFileOutputStream);
  outStream.init(testFile, 0x02 | 0x08 | 0x20, // write, create, truncate
                 0666, 0);

  File.createFromNsIFile(testFile).then(function(file) {
    sendAsyncMessage("emptyfile.opened", { file });
  });
});
