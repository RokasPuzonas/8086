class AssemblyViewElement extends HTMLElement {
	assemblySize = 0
	startAddress = 0

	connectedCallback() {
		this.classList.add("flex")
		this.addressColumn = document.createElement("div")
		this.addressColumn.classList.add("line_numbers")
		this.assemblyColumn = document.createElement("div")
		this.assemblyColumn.classList.add("assembly_code")
		this.appendChild(this.addressColumn)
		this.appendChild(this.assemblyColumn)
	}

	render() {
		const lineNumbers = []
		const assemblyLines = []

		let currentAddress = this.startAddress
		for (var i = 0; i < 1024; i++) {
			const assemblyDiv = document.createElement("div")
			let [assemblyStr, assemblySize] = getCurrentInstructionAt(currentAddress)
			assemblyDiv.textContent = assemblyStr
			assemblyLines.push(assemblyDiv)

			const addressDiv = document.createElement("div")
			addressDiv.textContent = "0x" + currentAddress.toString(16).padStart(4, "0")
			lineNumbers.push(addressDiv)

			currentAddress += assemblySize
			if (currentAddress >= this.assemblySize) break;
		}
		this.addressColumn.replaceChildren(...lineNumbers)
		this.assemblyColumn.replaceChildren(...assemblyLines)
	}
}

customElements.define("assembly-view", AssemblyViewElement)
