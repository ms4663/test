<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="14008000">
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="Commands" Type="Folder"/>
		<Item Name="Responses" Type="Folder"/>
		<Item Name="Checksum.vi" Type="VI" URL="../Checksum.vi"/>
		<Item Name="DevconUtil.vi" Type="VI" URL="../DevconUtil.vi"/>
		<Item Name="GetACK-NAK.vi" Type="VI" URL="../GetACK-NAK.vi"/>
		<Item Name="GetResponse.vi" Type="VI" URL="../GetResponse.vi"/>
		<Item Name="GetSnapshot.vi" Type="VI" URL="../GetSnapshot.vi"/>
		<Item Name="MIMIC_ResponseCluster.ctl" Type="VI" URL="../Responses/MIMIC_ResponseCluster.ctl"/>
		<Item Name="MIMIC_Test.vi" Type="VI" URL="../MIMIC_Test.vi"/>
		<Item Name="MIMICProtocol.vi" Type="VI" URL="../MIMICProtocol.vi"/>
		<Item Name="QueueMultiPakect_Snapshot.vi" Type="VI" URL="../QueueMultiPakect_Snapshot.vi"/>
		<Item Name="ReceiveMIMICCommand.vi" Type="VI" URL="../ReceiveMIMICCommand.vi"/>
		<Item Name="RescanUSB.vi" Type="VI" URL="../RescanUSB.vi"/>
		<Item Name="resize.vi" Type="VI" URL="../resize.vi"/>
		<Item Name="SaveImageFile.vi" Type="VI" URL="../SaveImageFile.vi"/>
		<Item Name="SerialCOM.vi" Type="VI" URL="../SerialCOM.vi"/>
		<Item Name="TransmitMIMICCommand.vi" Type="VI" URL="../TransmitMIMICCommand.vi"/>
		<Item Name="VerifyChecksum.vi" Type="VI" URL="../VerifyChecksum.vi"/>
		<Item Name="Dependencies" Type="Dependencies">
			<Item Name="vi.lib" Type="Folder">
				<Item Name="Calc Long Word Padded Width.vi" Type="VI" URL="/&lt;vilib&gt;/picture/bmp.llb/Calc Long Word Padded Width.vi"/>
				<Item Name="Clear Errors.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Clear Errors.vi"/>
				<Item Name="compatOverwrite.vi" Type="VI" URL="/&lt;vilib&gt;/_oldvers/_oldvers.llb/compatOverwrite.vi"/>
				<Item Name="Draw Flattened Pixmap.vi" Type="VI" URL="/&lt;vilib&gt;/picture/picture.llb/Draw Flattened Pixmap.vi"/>
				<Item Name="Empty Picture" Type="VI" URL="/&lt;vilib&gt;/picture/picture.llb/Empty Picture"/>
				<Item Name="Error Cluster From Error Code.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Error Cluster From Error Code.vi"/>
				<Item Name="FixBadRect.vi" Type="VI" URL="/&lt;vilib&gt;/picture/pictutil.llb/FixBadRect.vi"/>
				<Item Name="Flip and Pad for Picture Control.vi" Type="VI" URL="/&lt;vilib&gt;/picture/bmp.llb/Flip and Pad for Picture Control.vi"/>
				<Item Name="imagedata.ctl" Type="VI" URL="/&lt;vilib&gt;/picture/picture.llb/imagedata.ctl"/>
				<Item Name="LabVIEW Test - Test Data.ctl" Type="VI" URL="/&lt;vilib&gt;/addons/TestStand/_TSLegacy.llb/LabVIEW Test - Test Data.ctl"/>
				<Item Name="subTimeDelay.vi" Type="VI" URL="/&lt;vilib&gt;/express/express execution control/TimeDelayBlock.llb/subTimeDelay.vi"/>
				<Item Name="System Exec.vi" Type="VI" URL="/&lt;vilib&gt;/Platform/system.llb/System Exec.vi"/>
				<Item Name="TestStand - Create Test Data Cluster.vi" Type="VI" URL="/&lt;vilib&gt;/addons/TestStand/_TSLegacy.llb/TestStand - Create Test Data Cluster.vi"/>
				<Item Name="VISA Configure Serial Port" Type="VI" URL="/&lt;vilib&gt;/Instr/_visa.llb/VISA Configure Serial Port"/>
				<Item Name="VISA Configure Serial Port (Instr).vi" Type="VI" URL="/&lt;vilib&gt;/Instr/_visa.llb/VISA Configure Serial Port (Instr).vi"/>
				<Item Name="VISA Configure Serial Port (Serial Instr).vi" Type="VI" URL="/&lt;vilib&gt;/Instr/_visa.llb/VISA Configure Serial Port (Serial Instr).vi"/>
				<Item Name="VISA Flush IO Buffer Mask.ctl" Type="VI" URL="/&lt;vilib&gt;/Instr/_visa.llb/VISA Flush IO Buffer Mask.ctl"/>
				<Item Name="Write BMP Data To Buffer.vi" Type="VI" URL="/&lt;vilib&gt;/picture/bmp.llb/Write BMP Data To Buffer.vi"/>
				<Item Name="Write BMP Data.vi" Type="VI" URL="/&lt;vilib&gt;/picture/bmp.llb/Write BMP Data.vi"/>
				<Item Name="Write BMP File.vi" Type="VI" URL="/&lt;vilib&gt;/picture/bmp.llb/Write BMP File.vi"/>
			</Item>
		</Item>
		<Item Name="Build Specifications" Type="Build"/>
	</Item>
</Project>
