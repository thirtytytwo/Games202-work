class PRTMaterial extends Material {

    constructor(vertexShader, fragmentShader) {
        super({
            'uPrecomputeLR':{type: 'updateInRealTime', value: null},
            'uPrecomputeLG':{type: 'updateInRealTime', value: null},
            'uPrecomputeLB':{type: 'updateInRealTime', value: null},
        }, [
            'aPrecomputLT'
        ], vertexShader, fragmentShader, null);
    }
}

async function BuildPRTMaterial(vertexPath, fragmentPath) {


    let vertexShader = await getShaderString(vertexPath);
    let fragmentShader = await getShaderString(fragmentPath);

    return new PRTMaterial(vertexShader, fragmentShader);

}